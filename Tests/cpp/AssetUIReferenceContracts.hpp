#pragma once
#include <Workphone/Database/AssetDocumentInventory.hpp>
#include <Workphone/Database/MediaAssetCatalog.hpp>
#include <fstream>
#include <iterator>

namespace
{
    void assetUIReferenceContracts(const std::filesystem::path &folder)
    {
        namespace fs = std::filesystem;
        const auto root=folder/"ui-reference-graph";
        fs::create_directories(root/".workphone");
        auto catalog=make_ptr<AssetDatabaseManager>();
        require(catalog->setProjectRoot(root.u8string().c_str()),"UI reference project root");
        catalog->loadFromFile((root/".workphone/catalog.db").u8string().c_str());
        struct Close {SmartPtr<AssetDatabaseManager> catalog;~Close(){if(catalog)catalog->unload(nullptr);}} close{catalog};
        auto write=[](const fs::path &path,const std::string &text){
            fs::create_directories(path.parent_path());
            std::ofstream file(path,std::ios::binary|std::ios::trunc);
            file.write(text.data(),static_cast<std::streamsize>(text.size()));
            require(bool(file),"UI reference fixture write");
        };
        auto read=[](const fs::path &path){
            std::ifstream file(path,std::ios::binary);
            require(bool(file),"UI reference fixture read");
            return std::string(std::istreambuf_iterator<char>(file),{});
        };
        auto style=asset("style.uistyle"),locale=asset("strings.locres"),
             font=asset("face.fontres"),texture=asset("image.texres");
        for(auto object:{style,locale,font,texture}){
            catalog->addResourceEntry(object);
            write(root/fs::u8path(object->getFilePath().c_str()),"catalog target fixture");
        }
        const auto styleId=style->getHandle()->getUUIDAsString(),localeId=locale->getHandle()->getUUIDAsString(),
                   fontId=font->getHandle()->getUUIDAsString(),textureId=texture->getHandle()->getUUIDAsString();
        std::string upperStyle(styleId.c_str());
        for(auto &c:upperStyle)if(c>='a'&&c<='f')c-=('a'-'A');
        const std::string document=std::string("{\"version\":2,\"styleAssetUuid\":\"")+upperStyle+
            "\",\"localizationAssetUuid\":\""+localeId.c_str()+"\",\"locale\":\"fr\",\"nodes\":["+
            "{\"type\":\"container\",\"id\":1,\"parent\":-1,\"label\":\"HUD\",\"position\":[0,0],\"size\":[1,1],\"visible\":true,\"enabled\":true},"+
            "{\"type\":\"text\",\"id\":2,\"parent\":0,\"label\":\"Text\",\"position\":[0,0],\"size\":[1,1],\"visible\":true,\"enabled\":true,"+
            "\"fontAssetUuid\":\""+fontId.c_str()+"\",\"styleClass\":\"hud\",\"localeKey\":\"greeting\"},"+
            "{\"type\":\"image\",\"id\":3,\"parent\":0,\"label\":\"Image\",\"position\":[0,0],\"size\":[1,1],\"visible\":true,\"enabled\":true,"+
            "\"textureAssetUuid\":\""+textureId.c_str()+"\"}]}";
        write(root/"hud.uires",document);
        auto inventory=scanAssetDocuments(catalog);
        bool discovered=false;
        for(const auto &entry:inventory.documents)if(entry.path=="hud.uires")discovered=entry.canReindex&&entry.diagnostic.empty();
        require(inventory.succeeded&&discovered,"Discovery must validate an authored v2 UI document without Properties metadata");
        String error;u32 indexed=0;
        const bool reindexed=reindexAssetDocuments(catalog,inventory,indexed,error);
        require(reindexed&&indexed>=1,"Discovered UI document must reindex through the catalog graph");
        auto forward=queryAssetReferences(catalog,"hud.uires",false);
        require(forward.succeeded&&forward.edges.size()==4,"UI style, locale, font and texture must become durable required references");
        const auto checkTarget=[&](const String &id,const String &field,const String &path){
            const auto users=queryAssetReferences(catalog,id);
            require(users.succeeded&&users.edges.size()==1&&users.edges[0].sourcePath=="hud.uires"&&
                users.edges[0].targetUuid==id&&users.edges[0].required&&users.edges[0].diagnostic.empty()&&
                users.edges[0].expectedType=="workphone::IResource"&&users.edges[0].slot.find(field)!=String::npos,
                "UI where-used must retain canonical identity, field location and registered resource type");
            const auto blocked=catalog->performFileOperation(AssetDatabaseManager::FileOperation::Delete,path);
            require(!blocked.succeeded&&blocked.error.find("hud.uires")!=String::npos&&
                blocked.error.find(field)!=String::npos&&fs::exists(root/fs::u8path(path.c_str())),
                "A UI reference must block deletion with an actionable source and field");
        };
        checkTarget(styleId,"styleAssetUuid",style->getFilePath());
        checkTarget(localeId,"localizationAssetUuid",locale->getFilePath());
        checkTarget(fontId,"fontAssetUuid",font->getFilePath());
        checkTarget(textureId,"textureAssetUuid",texture->getFilePath());
        auto graphStamp=[&](){
            auto row=sql(catalog->getDatabase(),"SELECT revision,digest,state FROM wp_asset_reference_documents WHERE path_key='hud.uires'");
            require(row&&!row->eof(),"UI document graph stamp must exist");
            return row->getFieldValue("revision")+":"+row->getFieldValue("digest")+":"+row->getFieldValue("state");
        };
        const auto committed=graphStamp();
        auto duplicate=document;
        duplicate.insert(1,"\"version\":2,");
        auto invalid=document;
        invalid.replace(invalid.find(upperStyle),upperStyle.size(),"zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz");
        for(const auto &bad:{std::string("{\"version\":2,\"nodes\":["),duplicate,invalid}){
            write(root/"hud.uires",bad);
            error.clear();
            require(!reindexAssetDocument(catalog,"hud.uires",error)&&!error.empty(),
                "Malformed, duplicate-field and invalid-UUID UI must fail closed");
            auto blockedInventory=scanAssetDocuments(catalog);bool blocked=false;
            for(const auto &entry:blockedInventory.documents)if(entry.path=="hud.uires")blocked=!entry.canReindex&&!entry.diagnostic.empty();
            require(blockedInventory.succeeded&&blocked&&graphStamp()==committed&&
                queryAssetReferences(catalog,"hud.uires",false).edges.size()==4,
                "Invalid UI must preserve the complete last committed reference graph and revision");
            require(!catalog->performFileOperation(AssetDatabaseManager::FileOperation::Delete,"image.texres").succeeded&&
                fs::exists(root/"image.texres")&&read(root/"hud.uires")==bad,
                "Failed UI indexing must neither unblock destructive edits nor rewrite source bytes");
        }
        write(root/"hud.uires",document);
        require(read(root/"hud.uires")==document&&graphStamp()==committed,
            "UI reference discovery and indexing must never rewrite authored bytes");
        checkTarget(styleId,"styleAssetUuid",style->getFilePath());

        const auto mediaRoot=root/"bundled";
        write(mediaRoot/"icon.png","read-only media image");
        write(mediaRoot/"not-scanned.uires","invalid media document must remain excluded");
        auto media=openMediaAssetCatalog(mediaRoot.u8string().c_str(),error);
        require(media&&media->isReadOnly(),"UI media dependency must use an owned read-only catalog");
        Close closeMedia{media};
        const auto mediaEntries=media->queryEntries({});String mediaId;
        for(const auto &entry:mediaEntries.entries)if(entry.path=="icon.png")mediaId=entry.uuid;
        require(mediaEntries.succeeded&&!mediaId.empty(),"UI media texture must have a durable media UUID");
        const std::string mediaDocument=std::string("{\"version\":2,\"nodes\":[")+
            "{\"type\":\"container\",\"id\":1,\"parent\":-1,\"label\":\"Media\",\"position\":[0,0],\"size\":[1,1],\"visible\":true,\"enabled\":true},"+
            "{\"type\":\"image\",\"id\":2,\"parent\":0,\"label\":\"Icon\",\"position\":[0,0],\"size\":[1,1],\"visible\":true,\"enabled\":true,\"textureAssetUuid\":\""+
            mediaId.c_str()+"\"}]}";
        write(root/"media.uires",mediaDocument);
        inventory=scanAssetDocuments(catalog,{},media);discovered=false;
        for(const auto &entry:inventory.documents){
            require(entry.path!="bundled/not-scanned.uires","Mounted media UI must not become a project graph owner");
            if(entry.path=="media.uires")discovered=entry.canReindex;
        }
        const bool mediaIndexed=reindexAssetDocuments(catalog,inventory,indexed,error);
        require(inventory.succeeded&&discovered&&mediaIndexed,"Project UI may index captured read-only media references");
        const auto mediaUsers=queryAssetReferences(catalog,mediaId,true,"media");
        require(mediaUsers.succeeded&&mediaUsers.edges.size()==1&&mediaUsers.edges[0].sourcePath=="media.uires"&&
            mediaUsers.edges[0].targetCatalog=="media"&&mediaUsers.edges[0].required,
            "UI where-used must preserve the media catalog domain");
        require(!reindexAssetDocument(media,"not-scanned.uires",error)&&
            !media->performFileOperation(AssetDatabaseManager::FileOperation::Delete,"icon.png").succeeded&&
            read(mediaRoot/"icon.png")=="read-only media image"&&read(root/"media.uires")==mediaDocument,
            "UI reference work must not mutate a read-only media catalog or either source document");
        std::cout<<"PASS: typed UI discovery, durable required where-used, fail-closed malformed sources and read-only media references\n";
    }
}
