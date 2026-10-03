/*
 * Copyright (c) 2005 Erwin Coumans http://continuousphysics.com/Bullet/
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies.
 * Erwin Coumans makes no representations about the suitability
 * of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */
#ifndef CONTACT_SOLVER_INFO
#define CONTACT_SOLVER_INFO

struct ContactSolverInfo
{
    ContactSolverInfo()
    {
        m_tau = 1.0f;
        m_timeStep = 0.001f;
        m_restitution = 0.995f;
        m_maxErrorReduction = 0.5f;
        m_numIterations = 10;
        m_erp = 0.05f;
        m_sor = 1.5f;
    }

    float        m_tau;
    float        m_timeStep;
    float        m_restitution;
    unsigned int m_numIterations;
    float        m_maxErrorReduction;
    float        m_sor;
    float        m_erp;
};

#endif // CONTACT_SOLVER_INFO
