#include "RendererData.h"
    
void RendererData::minmaxAttributeGlb(const Attribute_t p)   
{
  MPI_Allreduce(&_attributeMinL[p], &_attributeMin[p], 1, MPI_FLOAT, MPI_MIN, comm);
  MPI_Allreduce(&_attributeMaxL[p], &_attributeMax[p], 1, MPI_FLOAT, MPI_MAX, comm);
}

void RendererData::randomShuffle()
{
  std::random_shuffle(data.begin(), data.end());
}
void RendererData::computeMinMax()
{
  _xminl=_yminl=_zminl=_rminl = +HUGE;
  _xmaxl=_ymaxl=_zmaxl=_rmaxl = -HUGE;
  for (int p = 0; p < NPROP; p++)
  {
    _attributeMinL[p] = +HUGE;
    _attributeMaxL[p] = -HUGE;
  }

  const int _n = data.size();
#pragma omp parallel
  {
    float xminl,yminl,zminl;
    float xmaxl,ymaxl,zmaxl;
    xminl=yminl=zminl=+HUGE;
    xmaxl=ymaxl=zmaxl=-HUGE;
    float attributeMinL[NPROP] = {+HUGE};
    float attributeMaxL[NPROP] = {-HUGE};

#pragma omp for schedule(static)
    for (int i = 0; i < _n; i++)
    {
      xminl = std::min(xminl, posx(i));
      yminl = std::min(yminl, posy(i));
      zminl = std::min(zminl, posz(i));
      xmaxl = std::max(xmaxl, posx(i));
      ymaxl = std::max(ymaxl, posy(i));
      zmaxl = std::max(zmaxl, posz(i));
      for (int p = 0; p < NPROP; p++)
      {
        attributeMinL[p] = std::min(attributeMinL[p], attribute(static_cast<Attribute_t>(p),i));
        attributeMaxL[p] = std::max(attributeMaxL[p], attribute(static_cast<Attribute_t>(p),i));
      }
    }

#pragma omp critical
    {
      _xminl = std::min(_xminl, xminl);
      _yminl = std::min(_yminl, yminl);
      _zminl = std::min(_zminl, zminl);
      _xmaxl = std::max(_xmaxl, xmaxl);
      _ymaxl = std::max(_ymaxl, ymaxl);
      _zmaxl = std::max(_zmaxl, zmaxl);
      for (int p = 0; p < NPROP; p++)
      {
        _attributeMinL[p] = std::min(_attributeMinL[p], attributeMinL[p]);
        _attributeMaxL[p] = std::max(_attributeMaxL[p], attributeMaxL[p]);
      }
    }
  }
  _rminl = std::min(_rminl, _xminl);
  _rminl = std::min(_rminl, _yminl);
  _rminl = std::min(_rminl, _zminl);
  _rmaxl = std::max(_rmaxl, _xmaxl);
  _rmaxl = std::max(_rmaxl, _ymaxl);
  _rmaxl = std::max(_rmaxl, _zmaxl);

#if 0
#pragma omp parallel for schedule(static)
  for (int i = 0; i < _n; i++)
  {
    assert(posx(i) >= _xminl && posx(i) <= _xmaxl);
    assert(posy(i) >= _yminl && posy(i) <= _ymaxl);
    assert(posz(i) >= _zminl && posz(i) <= _zmaxl);
    assert(posx(i) >= _rminl && posx(i) <= _rmaxl);
    assert(posy(i) >= _rminl && posy(i) <= _rmaxl);
    assert(posz(i) >= _rminl && posz(i) <= _rmaxl);
    for (int p = 0; p < NPROP; p++)
      assert(
          attribute(static_cast<Attribute_t>(p),i) >= _attributeMinL[p] &&
          attribute(static_cast<Attribute_t>(p),i) <= _attributeMaxL[p]);
  }
#endif


  float minloc[] = {_xminl, _yminl, _zminl, _rminl};
  float minglb[] = {_xminl, _yminl, _zminl, _rminl};

  float maxloc[] = {_xmaxl, _ymaxl, _zmaxl, _rmaxl};
  float maxglb[] = {_xmaxl, _ymaxl, _zmaxl, _rmaxl};

  MPI_Allreduce(minloc, minglb, 4, MPI_FLOAT, MPI_MIN, comm);
  MPI_Allreduce(maxloc, maxglb, 4, MPI_FLOAT, MPI_MAX, comm);

  _xmin = minglb[0];
  _ymin = minglb[1];
  _zmin = minglb[2];
  _rmin = minglb[3];
  _xmax = maxglb[0];
  _ymax = maxglb[1];
  _zmax = maxglb[2];
  _rmax = maxglb[3];

  for (int p = 0; p < NPROP; p++)
    minmaxAttributeGlb(static_cast<Attribute_t>(p));
}
    
template<typename Func>
void RendererData::rescale(const Attribute_t p, const Func &scale)
{
  float min = +HUGE, max = -HUGE;
  const int _n = data.size();
#pragma omp parallel for schedule(static) reduction(min:min) reduction(max:max)
  for (int i = 0; i < _n; i++)
  {
    attribute(p,i) = scale(attribute(p,i));
    min = std::min(min, attribute(p,i));
    max = std::max(max, attribute(p,i));
  }

  _attributeMinL[p] = min;
  _attributeMaxL[p] = max;

  _attributeMin[p] = scale(attributeMin(p));
  _attributeMax[p] = scale(attributeMax(p));
}

void RendererData::rescaleLinear(const Attribute_t p, const float newMin, const float newMax)
{
  const float oldMin = attributeMin(p);
  const float oldMax = attributeMax(p);

  const float oldRange = oldMax - oldMin ;
  assert(oldRange != 0.0);

  const float slope = (newMax - newMin)/oldRange;
  rescale(p,[&](const float x) { return slope * (x - oldMin) + newMin;});

}

void RendererData::scaleLog(const Attribute_t p, const float zeroPoint)
{
  rescale(p, [&](const float x) {return std::log(x + zeroPoint);});
}
void RendererData::scaleExp(const Attribute_t p, const float zeroPoint)
{
  rescale(p,[&](const float x) {return std::exp(x) - zeroPoint;});
}

void RendererData::clampMinMax(const Attribute_t p, const float min, const float max)
{
  rescale(p,[&](const float x) { return std::max(min, std::min(max, x)); });

  _attributeMin[p] = min;
  _attributeMax[p] = max;
}

