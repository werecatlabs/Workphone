namespace workphone
{
	namespace physics
	{

		//---------------------------------------------------------------------------------------------------
		inline bool WPPhysicsRigidBody2::_getFlag(u32 flag) const
		{
			return (m_staticState->m_flags & flag) != 0;
		}

		//---------------------------------------------------------------------------------------------------
		inline hash_type WPPhysicsRigidBody2::_getWorldId() const
		{
			return m_staticState->m_worldId;
		}

		//---------------------------------------------------------------------------------------------------
		inline real_Num WPPhysicsRigidBody2::getMassInv() const
		{
			return m_staticState->m_invMass;
		}

		//---------------------------------------------------------------------------------------------------
		inline real_Num WPPhysicsRigidBody2::getMass() const
		{
			return m_staticState->m_mass;
		}


	}
}


