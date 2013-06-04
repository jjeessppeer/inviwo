#ifndef IVW_IMAGE_H
#define IVW_IMAGE_H

#include <inviwo/core/common/inviwocoredefine.h>
#include <inviwo/core/datastructures/data.h>

namespace inviwo {

    class IVW_CORE_API Image : public Data2D {

    public:
        Image(uvec2 dimensions = uvec2(256,256), DataFormatBase format = DataVec4UINT8());
        virtual ~Image();
        void resize(uvec2 dimensions);
        virtual Data* clone() const;
        void resizeImageRepresentations(uvec2 targetDim);
    protected:
        void createDefaultRepresentation() const;
    };

} // namespace

#endif // IVW_IMAGE_H
