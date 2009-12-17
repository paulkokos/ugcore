//	created by Sebastian Reiter, Nicolas Tessore
//	s.b.reiter@googlemail.com
//	y08 m12 d02

#include "lib_grid/grid/grid.h"
#include "lib_grid/common_attachments.h"
#include "lib_grid/subset_handler_interface.h"

#ifndef __H__LIB_GRID__FILE_IO_NG__
#define __H__LIB_GRID__FILE_IO_NG__

namespace ug
{

bool ImportGridFromNG(Grid& grid,
                      const char* filename,
                      APosition& aPos = aPosition,
                      ISubsetHandler* pSubdomainHandler = NULL);

}//	end of namespace

#endif
