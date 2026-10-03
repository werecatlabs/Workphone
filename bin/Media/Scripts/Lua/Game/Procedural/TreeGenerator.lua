class 'TreeGenerator' (ProceduralObject)

-- A cless to generate a tree mesh for a graphics scene
function TreeGenerator:__init()
	ProceduralObject.__init(self)
	self.name = "TreeGenerator"
	self.description = "Generates a tree structure with branches and leaves."
	self.category = "Nature"
	self.icon = "tree_icon.png"
end