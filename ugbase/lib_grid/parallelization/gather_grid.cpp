/*
 * Copyright (c) 2017:  G-CSC, Goethe University Frankfurt
 * Author: Sebastian Reiter
 * 
 * This file is part of UG4.
 * 
 * UG4 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (2) The following notice must be displayed at a prominent place in the
 * terminal output of covered works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (3) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S., Vogel, A., Heppner, I., Rupp, M., and Wittum, G. A massively
 *   parallel geometric multigrid solver on hierarchically distributed grids.
 *   Computing and visualization in science 16, 4 (2013), 151-164"
 * "Vogel, A., Reiter, S., Rupp, M., Nägel, A., and Wittum, G. UG4 -- a novel
 *   flexible software system for simulating pde based models on high performance
 *   computers. Computing and visualization in science 16, 4 (2013), 165-179"
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#include "gather_grid.h"
#include "lib_grid/algorithms/selection_util.h"

namespace ug{

void GatherGrid(	Grid& gridOut,
					Selector& sel,
					GridDataSerializationHandler& serializer,
					GridDataSerializationHandler& deserializer,
					int root,
					const pcl::ProcessCommunicator& procCom)
{
	const int magicNumber = 68753267;

	UG_COND_THROW(sel.grid() == NULL, "A grid has to be assigned to the given selector.");
	Grid& gFrom = *sel.grid();
	SelectAssociatedGridObjects(sel);
//	serialize
	BinaryBuffer buf;
	Serialize(buf, magicNumber);
	SerializeGridElements(gFrom, sel.get_grid_objects(), buf);
	serializer.write_infos(buf);
	serializer.serialize(buf, sel.get_grid_objects());
	Serialize(buf, magicNumber);

//	communicate
	procCom.gather(buf, root);

//	deserialize
	gridOut.clear_geometry();
	Selector selNew(gridOut);
	selNew.enable_autoselection(true);
	const size_t numEntries = (procCom.get_local_proc_id() == root) ? procCom.size() : 1;

	for(size_t i = 0; i < numEntries; ++i){
		int tmp;
		Deserialize(buf, tmp);
		UG_COND_THROW(tmp != magicNumber, "Magic number mismatch in GatherGrid at check 1. "
					  "Internal error in grid serialization.");
		selNew.clear();
		DeserializeGridElements(gridOut, buf);
		deserializer.read_infos(buf);
		deserializer.deserialize(buf, selNew.get_grid_objects());
		Deserialize(buf, tmp);
		UG_COND_THROW(tmp != magicNumber, "Magic number mismatch in GatherGrid at check 2. "
					  "Please make sure to specify matching 'serializer' and 'deserializer'.");
	}
}


void AllGatherGrid(	Grid& gridOut,
					Selector& sel,
					GridDataSerializationHandler& serializer,
					GridDataSerializationHandler& deserializer,
					const pcl::ProcessCommunicator& procCom)
{
	const int magicNumber = 68753267;

	UG_COND_THROW(sel.grid() == NULL, "A grid has to be assigned to the given selector.");
	Grid& gFrom = *sel.grid();
	SelectAssociatedGridObjects(sel);
//	serialize
	BinaryBuffer buf;
	Serialize(buf, magicNumber);
	SerializeGridElements(gFrom, sel.get_grid_objects(), buf);
	serializer.write_infos(buf);
	serializer.serialize(buf, sel.get_grid_objects());
	Serialize(buf, magicNumber);

//	communicate
	procCom.allgather(buf);

//	deserialize
	gridOut.clear_geometry();
	Selector selNew(gridOut);
	selNew.enable_autoselection(true);

	for(size_t i = 0; i < procCom.size(); ++i){
		int tmp;
		Deserialize(buf, tmp);
		UG_COND_THROW(tmp != magicNumber, "Magic number mismatch in AllGather at check 1. "
					  "Internal error in grid serialization.");
		selNew.clear();
		DeserializeGridElements(gridOut, buf);
		deserializer.read_infos(buf);
		deserializer.deserialize(buf, selNew.get_grid_objects());
		Deserialize(buf, tmp);
		UG_COND_THROW(tmp != magicNumber, "Magic number mismatch in AllGather at check 2. "
					  "Please make sure to specify matching 'serializer' and 'deserializer'.");
	}
}

}//	end of namespace
