#include "PopCoreMl.h"
#include "SoyTypes.h"

namespace CoreMl
{
	const Soy::TVersion	Version(1, 1, 0);
}

__export int32_t PopCoreml_GetVersion()
{
	return CoreMl::Version.GetMillion();
}
