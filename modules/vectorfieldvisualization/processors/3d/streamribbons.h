/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2015 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#ifndef IVW_STREAMRIBBONS_H
#define IVW_STREAMRIBBONS_H

#include <modules/vectorfieldvisualization/vectorfieldvisualizationmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/ports/meshport.h>
#include <inviwo/core/properties/transferfunctionproperty.h>
#include <inviwo/core/datastructures/coordinatetransformer.h>
#include <modules/vectorfieldvisualization/streamlinetracer.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/optionproperty.h>
#include <inviwo/core/properties/boolproperty.h>

#include <modules/vectorfieldvisualization/ports/seedpointsport.h>

namespace inviwo {

/** \docpage{<classIdentifier>, StreamRibbons}
 * Explanation of how to use the processor.
 *
 * ### Inports
 *   * __<Inport1>__ <description>.
 *
 * ### Outports
 *   * __<Outport1>__ <description>.
 *
 * ### Properties
 *   * __<Prop1>__ <description>.
 *   * __<Prop2>__ <description>
 */

/**
 * \class StreamRibbons
 *
 * \brief <brief description>
 *
 * <Detailed description from a developer prespective>
 */
class IVW_MODULE_VECTORFIELDVISUALIZATION_API StreamRibbons : public Processor {
public:
    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;
    StreamRibbons();
    virtual ~StreamRibbons() {}

protected:
    virtual void process() override;

private:
    VolumeInport vectorVolume_;
    VolumeInport vorticityVolume_;
    SeedPointsInport seedPoints_;

    IntProperty numberOfSteps_;
    FloatProperty stepSize_;
    BoolProperty normalizeSamples_;
    TemplateOptionProperty<IntegralLineProperties::Direction> stepDirection_;
    TemplateOptionProperty<StructuredCoordinateTransformer<3>::Space> seedPointsSpace_;
    TemplateOptionProperty<IntegralLineProperties::IntegrationScheme> integrationScheme_;

    FloatProperty ribbonWidth_;

    TransferFunctionProperty tf_;
    FloatProperty velocityScale_;
    StringProperty maxVelocity_;
    StringProperty maxVorticity_;

    MeshOutport mesh_;
};

}  // namespace

#endif  // IVW_STREAMRIBBONS_H
