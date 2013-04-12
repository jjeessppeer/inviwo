#ifndef IVW_VOLUME_H
#define IVW_VOLUME_H

#include <inviwo/core/inviwocoredefine.h>
#include <inviwo/core/datastructures/data.h>
#include <inviwo/core/datastructures/volumerepresentation.h>

namespace inviwo {

    class IVW_CORE_API Volume : public Data3D {

    public:
        Volume();
        Volume(VolumeRepresentation*);
        Volume(VolumeRepresentation*, Volume*);
        virtual ~Volume();
        Data* clone() const;        
        void setOffset(ivec3); 
        ivec3 getOffset();
        DataFormatBase getDataFormat();
    protected:
        void createDefaultRepresentation();
    };

} // namespace

#endif // IVW_VOLUME_H
