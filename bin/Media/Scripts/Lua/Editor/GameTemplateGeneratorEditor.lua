class 'GameTemplateGeneratorEditor' 

function GameTemplateGeneratorEditor:__init(window)
    self.genre = nil;
    self.clearScene = true;
end

function GameTemplateGeneratorEditor:Open()
--[[
	GameTemplateGeneratorWindow window =
		GetWindow<GameTemplateGeneratorWindow>();

	window.titleContent = new GUIContent("Game Template");
	window.minSize = new Vector2(360, 220);
	window.Show();
	]]
end
	
 function GameTemplateGeneratorEditor:OnGUI()
 --[[
	GUILayout.Space(12);

	EditorGUILayout.LabelField(
		"Procedural Game Template Generator",
		EditorStyles.boldLabel
	);

	GUILayout.Space(8);

	genre = (GameTemplateGenre)EditorGUILayout.EnumPopup(
		"Genre",
		genre
	);

	clearScene = EditorGUILayout.Toggle(
		"Clear Current Scene",
		clearScene
	);

	GUILayout.Space(16);

	EditorGUILayout.HelpBox(
		"This will generate a starter scene hierarchy, player, camera, lighting, UI, and genre-specific placeholder objects.",
		MessageType.Info
	);

	GUILayout.FlexibleSpace();

	if (GUILayout.Button("Generate Template", GUILayout.Height(40)))
	{
		bool shouldGenerate = true;

		if (clearScene)
		{
			shouldGenerate = EditorUtility.DisplayDialog(
				"Generate Game Template",
				"This will clear the current scene before generating the template. Continue?",
				"Generate",
				"Cancel"
			);
		}

		if (shouldGenerate)
		{
			GameTemplateGenerator.Generate(genre, clearScene);
		}
	}

	GUILayout.Space(12);
	]]
end