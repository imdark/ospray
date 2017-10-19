// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "ToneMapperPixelOp.h"
#include "ToneMapperPixelOp_ispc.h"

namespace ospray {

  ToneMapperPixelOp::ToneMapperPixelOp()
  {
    ispcEquivalent = ispc::ToneMapperPixelOp_create();
  }

  void ToneMapperPixelOp::commit()
  {
    PixelOp::commit();
      
    float exposure = getParam1f("exposure", 1.f);
    
    // Default parameters fitted to the ACES 1.0 grayscale curve (RRT.a1.0.3 + ODT.Academy.Rec709_100nits_dim.a1.0.3)
    // We included exposure adjustment to match 18% middle gray (ODT(RRT(0.18)) = 0.18)
    const float acesContrast = 1.6773f;
    const float acesShoulder = 0.9714f;
    const float acesMidIn    = 0.18f;
    const float acesMidOut   = 0.18f;
    const float acesHdrMax   = 11.0785f;
    
    float a = max(getParam1f("contrast", acesContrast), 0.0001f);
    float d = clamp(getParam1f("shoulder", acesShoulder), 0.0001f, 1.f);
    float m = clamp(getParam1f("midIn", acesMidIn), 0.0001f, 1.f);
    float n = clamp(getParam1f("midOut", acesMidOut), 0.0001f, 1.f);
    float w = max(getParam1f("hdrMax", acesHdrMax), 1.f);
    
    // Solve b and c
    float b = -((pow(m,-a*d)*(-pow(m,a) + (n*(pow(m,a*d)*n*pow(w,a) - pow(m,a)*pow(w,a*d))) / (pow(m,a*d)*n - n*pow(w,a*d)))) / n);
    float c = max((pow(m,a*d)*n*pow(w,a) - pow(m,a)*pow(w,a*d)) / (pow(m,a*d)*n - n*pow(w,a*d)), 0.f); // avoid discontinuous curve by clamping to 0
    
    ispc::ToneMapperPixelOp_set(ispcEquivalent, exposure, a, b, c, d);
  }

  std::string ToneMapperPixelOp::toString() const
  {
    return "ospray::ToneMapperPixelOp";
  }

  PixelOp::Instance* ToneMapperPixelOp::createInstance(FrameBuffer* fb, PixelOp::Instance* prev)
  {
    // FIXME: use prev too
    return new ToneMapperPixelOp::Instance(getIE());
  }

  ToneMapperPixelOp::Instance::Instance(void* ispcInstance)
    : ispcInstance(ispcInstance)
  {
  }

  void ToneMapperPixelOp::Instance::postAccum(Tile& tile)
  {
    ToneMapperPixelOp_apply(ispcInstance, (ispc::Tile&)tile);
  }

  std::string ToneMapperPixelOp::Instance::toString() const
  {
    return "ospray::ToneMapperPixelOp::Instance";
  }

  OSP_REGISTER_PIXEL_OP(ToneMapperPixelOp, tonemapper);

} // ::ospray
