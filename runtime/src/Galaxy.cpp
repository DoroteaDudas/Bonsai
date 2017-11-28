/*
 * Galaxy.cpp
 *
 *  Created on: May 6, 2016
 *      Author: Bernd Doser <bernd.doser@h-its.org>
 */

#include <Galaxy.h>

real4 Galaxy::getCenterOfMass() const
{
  real mass;
  real4 result = make_real4(0.0, 0.0, 0.0, 0.0);
  for (auto const& p : pos)
  {
    mass = p.w;
    result.x += mass * p.x;
    result.y += mass * p.y;
    result.z += mass * p.z;
    result.w += mass;
  }

  result.x /= result.w;
  result.y /= result.w;
  result.z /= result.w;
  return result;
}

real4 Galaxy::getTotalVelocity() const
// use velocity of center-of-mass
{
  real mass;
  real4 result = make_real4(0.0, 0.0, 0.0, 0.0);
  for (int i = 0; i < vel.size(); i++)
  {
    mass = pos[i].w;
    result.x += mass * vel[i].x;
    result.y += mass * vel[i].y;
    result.z += mass * vel[i].z;
    result.w += mass;
  }

  result.x /= result.w;
  result.y /= result.w;
  result.z /= result.w;
  return result;
}

void Galaxy::centering()
{
  real4 center_of_mass = getCenterOfMass();
  for (auto &p : pos)
  {
    p.x -= center_of_mass.x;
    p.y -= center_of_mass.y;
    p.z -= center_of_mass.z;
  }
  std::cout << "  total mass: " << center_of_mass.w << std::endl;
}

void Galaxy::steady()
{
  real4 total_velocity = getTotalVelocity();
  for (auto &v : vel)
  {
    v.x -= total_velocity.x;
    v.y -= total_velocity.y;
    v.z -= total_velocity.z;
  }
}

void Galaxy::translate(real4 w)
{
  for (auto &p : pos)
  {
    p.x += w.x;
    p.y += w.y;
    p.z += w.z;
  }
}

void Galaxy::accelerate(real4 w)
{
  for (auto &v : vel)
  {
    v.x += w.x;
    v.y += w.y;
    v.z += w.z;
  }
}
