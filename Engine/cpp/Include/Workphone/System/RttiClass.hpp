#ifndef RttiClass_h__
#define RttiClass_h__

#define WP_OBJECT_CLASS_REGISTER_DECL   \
public:                                 \
    virtual void setTypeInfo( u32 id ); \
    static u32 typeInfo();              \
    static void setupTypeInfo();        \
    virtual u32 getTypeInfo() const;    \
    static u32 sTypeInfo;

#define WP_CLASS_REGISTER_DECL          \
public:                                 \
    virtual void setTypeInfo( u32 id ); \
    static u32 typeInfo();              \
    static void setupTypeInfo();        \
    virtual u32 getTypeInfo() const;    \
    static u32 sTypeInfo;

#define WP_CLASS_REGISTER_TEMPLATE_DECL( TEMPLATE_CLASS, TYPE ) \
public:                                                         \
    virtual void setTypeInfo( u32 id );                         \
    static u32 typeInfo();                                      \
    static void setupTypeInfo();                                \
    virtual u32 getTypeInfo() const;                            \
    static u32 sTypeInfo;

#define WP_CLASS_REGISTER_TEMPLATE_PAIR_DECL( TEMPLATE_CLASS, TYPE, SECOND_TYPE ) \
public:                                                                           \
    virtual void setTypeInfo( u32 id );                                           \
    static u32 typeInfo();                                                        \
    static void setupTypeInfo();                                                  \
    virtual u32 getTypeInfo() const;                                              \
    static u32 sTypeInfo;

#define WP_REGISTER_FACTORY_DECL   \
public:                            \
    static void registerFactory(); \
    static void unregisterFactory();

#define WP_DESCRIBE_CLASS( C, Bases, Public, Protected, Private ) \
    BOOST_DESCRIBE_CLASS( C, Bases, Public, Protected, Private )

#endif  // RttiClass_h__
