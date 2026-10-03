namespace workphone
{
	namespace physics
	{

		//---------------------------------------------------------------------------------------------------
		inline bool CRigidBody2::_getFlag(u32 flag) const
		{
			return (m_staticState->m_flags & flag) != 0;
		}

		//---------------------------------------------------------------------------------------------------
		inline hash_type CRigidBody2::_getWorldId() const
		{
			return m_staticState->m_worldId;
		}

		//---------------------------------------------------------------------------------------------------
		inline real_Num CRigidBody2::getMassInv() const
		{
			return m_staticState->m_invMass;
		}

		//---------------------------------------------------------------------------------------------------
		inline real_Num CRigidBody2::getMass() const
		{
			return m_staticState->m_mass;
		}


	}
}


