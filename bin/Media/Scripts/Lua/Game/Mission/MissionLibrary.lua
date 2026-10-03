include("MissionDefinition.lua")

MissionLibrary = MissionLibrary or {}

local function objective(values) return MissionObjective(values) end
local function position(x, y, z) return MissionRuntime.makeVector3(x, y, z) end

function MissionLibrary.createFirstFlight()
	return MissionDefinition({
		missionId = "first_flight",
		missionTitle = "First Flight",
		missionDescription = "Learn take-off, a stable climb, basic navigation, and landing.",
		category = "Training",
		difficulty = "Beginner",
		rewardScore = 750,
		tags = { "training", "takeoff", "landing" },
		objectives = {
			objective({ id = "first_takeoff", title = "Take Off", description = "Advance the throttle and establish a positive climb.", type = MissionObjectiveType.TakeOff, score = 100 }),
			objective({ id = "first_climb", title = "Climb to 50 m", description = "Climb smoothly to at least 50 metres above the field.", type = MissionObjectiveType.ReachAltitude, targetValue = 50.0, tolerance = 2.0, score = 150 }),
			objective({ id = "first_heading", title = "Hold Heading 090", description = "Maintain an easterly heading for five seconds.", type = MissionObjectiveType.MaintainHeading, targetValue = 90.0, tolerance = 12.0, duration = 5.0, score = 150 }),
			objective({ id = "first_waypoint", title = "Fly to the Practice Area", description = "Pass through the waypoint ahead of the runway.", type = MissionObjectiveType.GoToLocation, worldPosition = position(0.0, 50.0, 500.0), completionRadius = 30.0, score = 150 }),
			objective({ id = "first_land", title = "Land Safely", description = "Return to the field and make a controlled landing.", type = MissionObjectiveType.Land, metadata = { maximumVerticalSpeed = 4.0 }, score = 200 })
		}
	})
end

function MissionLibrary.createCircuitTraining()
	return MissionDefinition({
		missionId = "circuit_training",
		missionTitle = "Circuit Training",
		missionDescription = "Fly a complete rectangular traffic pattern and land on the active runway.",
		category = "Training",
		difficulty = "Intermediate",
		rewardScore = 1100,
		tags = { "circuit", "navigation", "landing" },
		objectives = {
			objective({ id = "circuit_takeoff", title = "Take Off", type = MissionObjectiveType.TakeOff, score = 100 }),
			objective({ id = "circuit_upwind", title = "Upwind Gate", type = MissionObjectiveType.GoToLocation, worldPosition = position(0.0, 75.0, 650.0), completionRadius = 35.0, score = 150 }),
			objective({ id = "circuit_crosswind", title = "Crosswind Gate", type = MissionObjectiveType.GoToLocation, worldPosition = position(400.0, 75.0, 650.0), completionRadius = 35.0, score = 150 }),
			objective({ id = "circuit_downwind", title = "Downwind Gate", type = MissionObjectiveType.GoToLocation, worldPosition = position(400.0, 75.0, 0.0), completionRadius = 35.0, score = 200 }),
			objective({ id = "circuit_base", title = "Base Leg", type = MissionObjectiveType.GoToLocation, worldPosition = position(400.0, 45.0, -350.0), completionRadius = 35.0, score = 150 }),
			objective({ id = "circuit_final", title = "Final Approach", type = MissionObjectiveType.GoToLocation, worldPosition = position(0.0, 25.0, -350.0), completionRadius = 30.0, score = 150 }),
			objective({ id = "circuit_land", title = "Touch Down", type = MissionObjectiveType.TouchdownZone, worldPosition = position(0.0, 0.0, 0.0), completionRadius = 45.0, metadata = { maximumVerticalSpeed = 3.5 }, score = 200 })
		}
	})
end

function MissionLibrary.createNavigationChallenge()
	return MissionDefinition({
		missionId = "navigation_challenge",
		missionTitle = "Navigation Challenge",
		missionDescription = "Navigate a sequence of distant waypoints while maintaining safe altitude.",
		category = "Navigation",
		difficulty = "Intermediate",
		timeLimit = 600.0,
		rewardScore = 1400,
		tags = { "navigation", "waypoints", "timed" },
		objectives = {
			objective({ id = "nav_takeoff", title = "Depart the Airfield", type = MissionObjectiveType.TakeOff, score = 100 }),
			objective({ id = "nav_climb", title = "Reach Cruise Altitude", type = MissionObjectiveType.ReachAltitude, targetValue = 120.0, score = 150 }),
			objective({ id = "nav_wp1", title = "Waypoint Alpha", type = MissionObjectiveType.GoToLocation, worldPosition = position(800.0, 120.0, 900.0), completionRadius = 45.0, score = 250 }),
			objective({ id = "nav_wp2", title = "Waypoint Bravo", type = MissionObjectiveType.GoToLocation, worldPosition = position(-650.0, 140.0, 1500.0), completionRadius = 45.0, score = 250 }),
			objective({ id = "nav_wp3", title = "Waypoint Charlie", type = MissionObjectiveType.GoToLocation, worldPosition = position(-900.0, 100.0, 400.0), completionRadius = 45.0, score = 250 }),
			objective({ id = "nav_return", title = "Return to Base", type = MissionObjectiveType.GoToLocation, worldPosition = position(0.0, 60.0, -250.0), completionRadius = 60.0, score = 200 }),
			objective({ id = "nav_land", title = "Land", type = MissionObjectiveType.Land, score = 200 })
		}
	})
end

function MissionLibrary.createPrecisionLanding()
	return MissionDefinition({
		missionId = "precision_landing",
		missionTitle = "Precision Landing",
		missionDescription = "Fly a stabilised approach and touch down inside the marked zone.",
		category = "Landing",
		difficulty = "Advanced",
		rewardScore = 900,
		tags = { "approach", "precision", "landing" },
		objectives = {
			objective({ id = "landing_approach", title = "Intercept Final", type = MissionObjectiveType.GoToLocation, worldPosition = position(0.0, 55.0, -700.0), completionRadius = 35.0, score = 200 }),
			objective({ id = "landing_speed", title = "Stabilise Approach Speed", type = MissionObjectiveType.ReachSpeed, targetValue = 65.0, tolerance = 10.0, metadata = { comparison = "atMost" }, score = 150 }),
			objective({ id = "landing_zone", title = "Touch Down in the Zone", type = MissionObjectiveType.TouchdownZone, worldPosition = position(0.0, 0.0, 0.0), completionRadius = 25.0, metadata = { maximumVerticalSpeed = 2.5 }, score = 450 }),
			objective({ id = "landing_stop", title = "Come to a Stop", type = MissionObjectiveType.ReachSpeed, targetValue = 2.0, tolerance = 1.0, metadata = { comparison = "atMost" }, score = 100 })
		}
	})
end

function MissionLibrary.createAerobaticsQualification()
	return MissionDefinition({
		missionId = "aerobatics_qualification",
		missionTitle = "Aerobatics Qualification",
		missionDescription = "Demonstrate safe altitude control and complete the required manoeuvres.",
		category = "Aerobatics",
		difficulty = "Advanced",
		rewardScore = 1500,
		tags = { "aerobatics", "roll", "loop" },
		objectives = {
			objective({ id = "aero_takeoff", title = "Take Off", type = MissionObjectiveType.TakeOff, score = 100 }),
			objective({ id = "aero_altitude", title = "Enter the Aerobatic Box", type = MissionObjectiveType.ReachAltitude, targetValue = 100.0, score = 150 }),
			objective({ id = "aero_loop", title = "Complete a Loop", type = MissionObjectiveType.AerobaticManeuver, targetId = "loop", score = 350 }),
			objective({ id = "aero_roll", title = "Complete an Aileron Roll", type = MissionObjectiveType.AerobaticManeuver, targetId = "aileron_roll", score = 350 }),
			objective({ id = "aero_inverted", title = "Hold Inverted Flight", type = MissionObjectiveType.Custom, targetId = "inverted_hold", duration = 3.0, score = 300 }),
			objective({ id = "aero_land", title = "Return and Land", type = MissionObjectiveType.Land, score = 250 })
		}
	})
end

function MissionLibrary.createSearchAndRescue()
	return MissionDefinition({
		missionId = "search_and_rescue",
		missionTitle = "Search and Rescue",
		missionDescription = "Locate the incident, confirm the survivor, and return safely.",
		category = "Operations",
		difficulty = "Advanced",
		timeLimit = 900.0,
		rewardScore = 1600,
		tags = { "search", "rescue", "navigation" },
		objectives = {
			objective({ id = "sar_takeoff", title = "Launch", type = MissionObjectiveType.TakeOff, score = 100 }),
			objective({ id = "sar_search", title = "Reach the Search Area", type = MissionObjectiveType.GoToLocation, worldPosition = position(1200.0, 90.0, 1800.0), completionRadius = 100.0, score = 300 }),
			objective({ id = "sar_locate", title = "Locate the Survivor", type = MissionObjectiveType.Custom, targetId = "survivor_located", score = 350 }),
			objective({ id = "sar_rescue", title = "Complete the Rescue", type = MissionObjectiveType.Custom, targetId = "rescue_complete", score = 400 }),
			objective({ id = "sar_return", title = "Return to Base", type = MissionObjectiveType.GoToLocation, worldPosition = position(0.0, 60.0, -200.0), completionRadius = 70.0, score = 250 }),
			objective({ id = "sar_land", title = "Land Safely", type = MissionObjectiveType.Land, score = 200 })
		}
	})
end

function MissionLibrary.getBuiltIns()
	return {
		MissionLibrary.createFirstFlight(),
		MissionLibrary.createCircuitTraining(),
		MissionLibrary.createNavigationChallenge(),
		MissionLibrary.createPrecisionLanding(),
		MissionLibrary.createAerobaticsQualification(),
		MissionLibrary.createSearchAndRescue()
	}
end

function MissionLibrary.registerWith(manager)
	if not manager then return 0 end
	local count = 0
	for _, mission in ipairs(MissionLibrary.getBuiltIns()) do
		local called, result = MissionRuntime.tryCall(manager, { "registerMission", "RegisterMission" }, mission)
		if called and result ~= false then count = count + 1 end
	end
	return count
end

MissionLibrary.GetBuiltIns = MissionLibrary.getBuiltIns
MissionLibrary.RegisterWith = MissionLibrary.registerWith

return MissionLibrary
