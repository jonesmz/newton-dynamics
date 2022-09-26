package com.javaNewton;

import com.newton.nRigidBodyType;
import com.newton.ndRigidBodyGlue;
import com.newton.ndBodyNotifyGlue;
import com.newton.ndShapeInstanceGlue;

public class nRigidBody extends ndRigidBodyGlue
{
    private ndBodyNotifyGlue m_notify;
    private ndShapeInstanceGlue m_shapeInstance;

    public nRigidBody(nRigidBodyType type)
    {
        super(type);
    }

    @Override
    public void SetNotifyCallback(ndBodyNotifyGlue notifyCallBack)
    {
        m_notify = notifyCallBack;
        super.SetNotifyCallback(m_notify);
    }

    public void SetCollisionShape(ndShapeInstanceGlue shapeInstance)
    {
        super.SetCollisionShape(shapeInstance);
        //m_shapeInstance = new ndShapeInstanceGlue(GetCollisionShape().GetShape());
    }
}
