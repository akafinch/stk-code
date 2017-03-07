//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2012-2015 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "karts/cannon_animation.hpp"

#include "animations/animation_base.hpp"
#include "animations/ipo.hpp"
#include "animations/three_d_animation.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/kart_properties.hpp"
#include "modes/world.hpp"

#include "LinearMath/btTransform.h"

CannonAnimation::CannonAnimation(AbstractKart *kart, Ipo *ipo,
                                 const Vec3 &start_left, const Vec3 &start_right,
                                 const Vec3 &end_left,   const Vec3 &end_right   )
               : AbstractKartAnimation(kart, "CannonAnimation")
{
    m_curve  = new AnimationBase(ipo);
    m_timer  = ipo->getEndTime();

    float kw2 = m_kart->getKartModel()->getWidth()*0.5f;

    // First make sure that left and right points are indeed correct
    Vec3 my_start_left = start_left;
    Vec3 my_start_right = start_right;
    Vec3 p0, p1;
    m_curve->getAt(0, &p0);
    m_curve->getAt(0.1f, &p1);
    Vec3 p2 = 0.5f*(p0 + p1) + m_kart->getNormal();
    if (start_left.sideofPlane(p0, p1, p2) < 0)
    {
        my_start_left = start_right;
        my_start_right = start_left; 
    }
    // First adjust start and end points to take on each side half the kart
    // width into account:
    Vec3 direction = my_start_right - my_start_left;
    direction.normalize();

    Vec3 adj_start_left  = my_start_left  + kw2 * direction;
    Vec3 adj_start_right = my_start_right - kw2 * direction;

    // Same adjustments for end points
    float t = m_curve->getAnimationDuration();
    Vec3 my_end_left = end_left;
    Vec3 my_end_right = end_right;
    m_curve->getAt(t-0.1f, &p0);
    m_curve->getAt(t, &p1);
    p2 = 0.5f*(p0 + p1) + m_kart->getNormal();
    if (end_left.sideofPlane(p0, p1, p2) < 0)
    {
        my_end_left = end_right;
        my_end_right = end_left; 
    }
    // Left and right end points are sometimes swapped
    direction = my_end_right - my_end_left;
    direction.normalize();

    Vec3 adj_end_left  = my_end_left  + kw2 * direction;
    Vec3 adj_end_right = my_end_right - kw2 * direction;

    // The kart position is divided into three components:
    // 1) The point at the curve at t=0.
    // 2) A component parallel to the start line. This component is scaled
    //    depending on time, length of start- and end-line (e.g. if the 
    //    end line is twice as long as the start line, this will make sure
    //    that a kart starting at the very left of the start line will end
    //    up at the very left of the end line. This component can also be
    //    adjusted by steering while in the air. This is done by modifying
    //    m_fraction_of_line, which is multiplied with the current width
    //    vector.
    // 3) The rest, i.e. the amoount that the kart is ahead and above the
    //    start line. This is stored in m_delta.
    // 
    // Compute the delta between the kart position and the start of the curve.
    // This delta is rotated with the kart and added to the interpolated curve
    // position to get the actual kart position during the animation.

    Vec3 curve_xyz;
    m_curve->update(0, &curve_xyz);
    m_delta = kart->getXYZ() - curve_xyz;

    m_start_line = 0.5f*(adj_start_right - adj_start_left);
    m_end_line   = 0.5f*(adj_end_right   - adj_end_left  );
    
    Vec3 v = adj_start_left - adj_start_right;
    float l = v.length();
    v /= l;

    // Compute on which fraction of the start line the kart is
    float f = v.dot(adj_start_left - kart->getXYZ());
    if (f <= 0)
        f = 0;
    else if (f >= l)
        f = l;
    else
        f = f / l;
    // Now f is in [0,1]. Convert it to [-1,1] assuming that the
    // ipo for the cannon is in the middle of the start and end line
    m_fraction_of_line = 2.0f*f - 1.0f;

    Vec3 delta = m_start_line * m_fraction_of_line;
    m_delta = m_delta - delta;
    
    // The previous call to m_curve->update will set the internal timer
    // of the curve to dt. Reset it to 0 to make sure the timer is in
    // synch with the timer of the CanonAnimation
    m_curve->reset();
}   // CannonAnimation

// ----------------------------------------------------------------------------
CannonAnimation::~CannonAnimation()
{
    delete m_curve;

    btTransform pos;
    pos.setOrigin(m_kart->getXYZ());
    //pos.setRotation(btQuaternion(btVector3(0.0f, 1.0f, 0.0f),
    //                             m_kart->getHeading()        ));
    pos.setRotation(m_kart->getRotation());

    m_kart->getBody()->setCenterOfMassTransform(pos);
    Vec3 v(0, 0, m_kart->getKartProperties()->getEngineMaxSpeed());
    m_kart->setVelocity(pos.getBasis()*v);
}   // ~CannonAnimation

// ----------------------------------------------------------------------------
/** Updates the kart animation.
 *  \param dt Time step size.
 *  \return True if the explosion is still shown, false if it has finished.
 */
void CannonAnimation::update(float dt)
{
    if(m_timer < dt)
    {
        AbstractKartAnimation::update(dt);
        return;
    }

    // Adjust the horizontal location based on steering
    m_fraction_of_line += m_kart->getSteerPercent()*dt*2.0f;

    // The timer count backwards, so the fraction goes from 1 to 0
    float f = m_timer / m_curve->getAnimationDuration();

    btClamp(m_fraction_of_line, -1.0f, 1.0f);
    Vec3 current_width = m_start_line * f + m_end_line * (1.0f - f);

    // Get the tangent = derivative at the current point to compute the
    // orientation of the kart
    Vec3 tangent;
    m_curve->getDerivativeAt(m_curve->getAnimationDuration() - m_timer,
                             &tangent);

    Vec3 forward = m_kart->getTrans().getBasis().getColumn(2);
    forward.normalize();

    // Only adjust the heading. I tried to also adjust pitch,
    // but that adds a strong roll to the kart on some cannons
    Vec3 v1(tangent), v2(forward);
    v1.setY(0); v2.setY(0);
    btQuaternion q = m_kart->getRotation()*shortestArcQuatNormalize2(v2, v1);

    m_kart->setRotation(q);

    Vec3 xyz;
    m_curve->update(dt, &xyz);

    Vec3 rotated_delta = quatRotate(q, m_delta ) + current_width * m_fraction_of_line;
    m_kart->setXYZ(xyz+rotated_delta);
   // m_kart->setXYZ(xyz);


    AbstractKartAnimation::update(dt);
}   // update
