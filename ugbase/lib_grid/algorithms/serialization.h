// created by Sebastian Reiter
// y09 m11 d04
// s.b.reiter@googlemail.com

#ifndef __H__LIB_GRID__SERIALIZATION__
#define __H__LIB_GRID__SERIALIZATION__

#include <iostream>
#include "lib_grid/lg_base.h"
#include "callbacks/callback_definitions.h"

namespace ug
{

////////////////////////////////////////////////////////////////////////
//	Utilities

/**	\brief	Interface for handling serialization and deserialization of
 * 			data associated with geometric objects.
 *
 * The GeomObjDataSerializer allows to serialize data associated with
 * geometric objects. Before the data will be serialized, write_info is
 * called. Accordingly read_info is called before data is deserialized.
 *
 * Note that this class handles serialization and deserialization at once.
 *
 * Make sure to completely read all data written by the associated write calls.
 *
 * Note that the following typedefs exist: VertexDataSerializer,
 * EdgeDataSerializer, FaceDataSerializer, VolumeDataSerializer.
 *
 * If one wants to serialize data of all objects in a grid, he should
 * take a look at GridDataSerializer.
 */
template <class TGeomObj>
class GeomObjDataSerializer
{
	public:
		virtual ~GeomObjDataSerializer()	{}

	///	can be used to write arbitrary info to the file.
	/**	Make sure to read everything you've written during read_data.
	 * Default implementation is empty.*/
		virtual void write_info(std::ostream& out) const {};
	///	write data associated with the given object. Pure virtual.
		virtual void write_data(std::ostream& out, TGeomObj* o) const = 0;

	///	Read the info written during write_info here. Default: empty implementation.
		virtual void read_info(std::istream& in) const	{};
	///	read data associated with the given object. Pure virtual.
		virtual void read_data(std::istream& in, TGeomObj* o) const = 0;
};

typedef GeomObjDataSerializer<VertexBase>	VertexDataSerializer;
typedef GeomObjDataSerializer<EdgeBase>		EdgeDataSerializer;
typedef GeomObjDataSerializer<Face>			FaceDataSerializer;
typedef GeomObjDataSerializer<Volume>		VolumeDataSerializer;

/**	\brief	Interface for handling serialization and deserialization of
 * 			data associated with all geometric objects in a grid.
 *
 * The GridDataSerializer allows to serialize data associated with
 * all geometric objects in a grid. Before the data will be serialized,
 * write_info is called. Accordingly read_info is called before data is
 * deserialized.
 *
 * Note that this class handles serialization and deserialization at once.
 *
 * Make sure to completely read all data written by the associated write calls.
 *
 * Note that this class specifies the interfaces of VertexDataSerializer,
 * EdgeDataSerializer, FaceDataSerializer, VolumeDataSerializer.
 *
 * All methods have an empty implementation by default.
 */
class GridDataSerializer : public virtual VertexDataSerializer,
	EdgeDataSerializer, FaceDataSerializer, VolumeDataSerializer
{
	public:
	///	can be used to write arbitrary info to the file.
	/**	Make sure to read everything you've written during read_data.
	 * Default implementation is empty.*/
		virtual void write_info(std::ostream& out) const				{}

	///	Read the info written during write_info here. Default: empty implementation.
		virtual void read_info(std::istream& in) const					{}

		virtual void write_data(std::ostream& out, VertexBase* o) const	{}
		virtual void write_data(std::ostream& out, EdgeBase* o) const	{}
		virtual void write_data(std::ostream& out, Face* o) const		{}
		virtual void write_data(std::ostream& out, Volume* o) const		{}

		virtual void read_data(std::istream& in, VertexBase* o) const	{}
		virtual void read_data(std::istream& in, EdgeBase* o) const		{}
		virtual void read_data(std::istream& in, Face* o) const			{}
		virtual void read_data(std::istream& in, Volume* o) const		{}
};



///	Serialization of data associated with grid elements.
/**	Through the add-method callback-classes can be registered,
 * which will be called during serialization and deserialization to
 * write data associated with the given elements into a binary stream.
 *
 * Note when data for a given object-type not only registered
 * callback classes for the type are called, but also registered
 * instances of GridDataSerializer.
 *
 * Note that this class performs both serialization and deserialization.
 */
class GridDataSerializationHandler
{
	public:
		~GridDataSerializationHandler()	{}

	/**	\{
	 * Adds a callback class for serialization and deserialization.
	 * Note that only a pointer to those callback-classes is stored.
	 * You thus have to make sure that this pointer points to a
	 * valid instance until the whole serialization and deserialization
	 * process is done.
	 */
		void add(VertexDataSerializer* cb);
		void add(EdgeDataSerializer* cb);
		void add(FaceDataSerializer* cb);
		void add(VolumeDataSerializer* cb);
		void add(GridDataSerializer* cb);
	/**	\} */


	///	calls write_info on all registered serializers
		void write_infos(std::ostream& out) const;

	/**	\{
	 * \brief Serializes data associated with the given object.*/
		inline void serialize(std::ostream& out, VertexBase* vrt) const;
		inline void serialize(std::ostream& out, EdgeBase* edge) const;
		inline void serialize(std::ostream& out, Face* face) const;
		inline void serialize(std::ostream& out, Volume* vol) const;
	/**	\} */

	///	Calls serialize on all elements between begin and end.
	/**	Make sure that TIterator::value_type is compatible with
	 * either VertexBase*, EdgeBase*, Face*, Volume*.*/
		template <class TIterator>
		void serialize(std::ostream& out, TIterator begin, TIterator end) const;


	///	calls read_info on all registered serializers
		void read_infos(std::istream& in) const;

	/**	\{
	 * \brief Deserializes data associated with the given object.*/
		inline void deserialize(std::istream& in, VertexBase* vrt) const;
		inline void deserialize(std::istream& in, EdgeBase* edge) const;
		inline void deserialize(std::istream& in, Face* face) const;
		inline void deserialize(std::istream& in, Volume* vol) const;
	/**	\} */

	///	Calls deserialize on all elements between begin and end.
	/**	Make sure that TIterator::value_type is compatible with
	 * either VertexBase*, EdgeBase*, Face*, Volume*.*/
		template <class TIterator>
		void deserialize(std::istream& in, TIterator begin, TIterator end) const;

	private:
	///	performs serialization on all given serializers.
		template<class TGeomObj, class TSerializers>
		void serialize(std::ostream& out, TGeomObj* o,
					   TSerializers& serializers) const;

	///	performs deserialization on all given deserializers.
		template<class TGeomObj, class TDeserializers>
		void deserialize(std::istream& in, TGeomObj* o,
					   TDeserializers& deserializers) const;

		template<class TSerializers>
		void write_info(std::ostream& out, TSerializers& serializers) const;

		template<class TSerializers>
		void read_info(std::istream& in, TSerializers& serializers) const;

	private:
		std::vector<VertexDataSerializer*>	m_vrtSerializers;
		std::vector<EdgeDataSerializer*>	m_edgeSerializers;
		std::vector<FaceDataSerializer*>	m_faceSerializers;
		std::vector<VolumeDataSerializer*>	m_volSerializers;
		std::vector<GridDataSerializer*>	m_gridSerializers;
};


////////////////////////////////////////////////////////////////////////
///	Serialization callback for grid attachments
/**	template class where TGeomObj should be one of the
 * following types: VertexBase, EdgeBase, Face, Volume.
 *
 * Note that the attachment is automatically attached, if not yet present.
 */
template <class TGeomObj, class TAttachment>
class GeomObjAttachmentSerializer :
	public GeomObjDataSerializer<TGeomObj>
{
	public:
		GeomObjAttachmentSerializer(Grid& g, TAttachment& a) :
			m_aa(g, a, true)	{}

		virtual void write_data(std::ostream& out, TGeomObj* o) const
		{Serialize(out, m_aa[o]);}

		virtual void read_data(std::istream& in, TGeomObj* o) const
		{Deserialize(in, m_aa[o]);}

	private:
		Grid::AttachmentAccessor<TGeomObj, TAttachment>	m_aa;
};

class SubsetHandlerSerializer : public GridDataSerializer
{
	public:
		SubsetHandlerSerializer(ISubsetHandler& sh);

	///	writes subset-infos to the stream (subset names and colors)
		virtual void write_info(std::ostream& out) const;

		///	Read the info written during write_info here. Default: empty implementation.
		virtual void read_info(std::istream& in) const;

		virtual void write_data(std::ostream& out, VertexBase* o) const;
		virtual void write_data(std::ostream& out, EdgeBase* o) const;
		virtual void write_data(std::ostream& out, Face* o) const;
		virtual void write_data(std::ostream& out, Volume* o) const;

		virtual void read_data(std::istream& in, VertexBase* o) const;
		virtual void read_data(std::istream& in, EdgeBase* o) const;
		virtual void read_data(std::istream& in, Face* o) const;
		virtual void read_data(std::istream& in, Volume* o) const;

	private:
		ISubsetHandler& m_sh;
};


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	GRID

////////////////////////////////////////////////////////////////////////
///	Writes a part of the grids elements to a binary-stream.
/**
 * The passed GeometricObjectCollection goc may only reference
 * elements of the given grid. It is important, that the goc
 * is complete - that means that all referenced vertices are
 * contained in the goc.
 *
 * If you pack several different parts of your grid, you should use
 * this method, since it is faster than calling SerializeGridElements
 * without the attachment.
 *
 * the integer attachment aInt is used during this method to store
 * an index in each vertex of the goc. The initial content of
 * the referenced attachment is ignored.
 *
 * The caller is responsible to attach the aIntVRT attachment to the 
 * to the vertices of the grid before calling this method.
 * The caller is also responsible to detach aIntVRT from the grids
 * vertices when it is no longer required.
 *
 * After termination the attachment holds the indices at which
 * the respcetive vertices are stored in the pack.
 */
bool SerializeGridElements(Grid& grid, GeometricObjectCollection goc,
						   AInt& aIntVRT, std::ostream& out);

////////////////////////////////////////////////////////////////////////
///	Writes all grid elements into a binary-stream.
bool SerializeGridElements(Grid& grid, std::ostream& out);

////////////////////////////////////////////////////////////////////////
///	Writes a part of the grids elements to a binary-stream.
/**
 * The passed GeometricObjectCollection goc may only reference
 * elements of the given grid. It is important, that the goc
 * is complete - that means that all referenced vertices are
 * contained in the goc.
 *
 * If you're planning to serialize multiple parts of one grid, you
 * should consider to use the full-featured serialization method.
 */
bool SerializeGridElements(Grid& grid, GeometricObjectCollection goc,
						   std::ostream& out);

////////////////////////////////////////////////////////////////////////
///	Creates grid elements from a binary stream
/**	Old versions of SerializeGrid did not support grid-headers.
 * This is why you can specify via readGridHeader whether a
 * header should be read (default is true).
 */
bool DeserializeGridElements(Grid& grid, std::istream& in,
							bool readGridHeader = true);


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	MULTI-GRID

////////////////////////////////////////////////////////////////////////
//	SerializeMultiGridElements
///	writes a part of the elements of a MultiGrid to a binary stream.
/**
 * THIS METHOD USES Grid::mark.
 *
 * The passed GeometricObjectCollection goc may only
 * reference elements of the given grid. It is important, that the goc
 * is complete - that means that all referenced vertices are
 * contained in the goc.
 * The goc has also to be complete in regard to the multi-grid hierarchy.
 * This means that for each element of the goc, the parent has to be
 * also part of the goc.
 *
 * If you pack several different parts of your grid, you should use
 * this method, since it is faster than calling SerializeGridElements
 * without the attachment.
 *
 * the integer attachment aInt is used during this method to store
 * an index in each element of the goc. The initial content of
 * the referenced attachment is ignored.
 *
 * The caller is responsible to attach the aIntVRT attachment to the 
 * to the vertices of the grid before calling this method.
 * The caller is also responsible to detach aIntVRT from the grids
 * vertices when it is no longer required.
 *
 * After termination the attachments hold the indices that were
 * assigned to the respective elements - starting from 0 for each
 * element type.
 */
bool SerializeMultiGridElements(MultiGrid& mg,
								GeometricObjectCollection goc,
								AInt& aIntVRT, AInt& aIntEDGE,
								AInt& aIntFACE, AInt& aIntVOL,
								std::ostream& out);

////////////////////////////////////////////////////////////////////////
//	SerializeMultiGridElements
///	writes a part of the elements of a MultiGrid to a binary stream.
/**
 * The passed GeometricObjectCollection goc may only reference
 * elements of the given grid. It is important, that the goc
 * is complete - that means that all referenced vertices are
 * contained in the goc.
 * The goc has also to be complete in regard to the multi-grid hierarchy.
 * This means that for each element of the goc, the parent has to be
 * also part of the goc.
 *
 * If you're planning to serialize multiple parts of one grid, you
 * should consider to use the full-featured serialization method.
 */
bool SerializeMultiGridElements(MultiGrid& mg,
								GeometricObjectCollection goc,
								std::ostream& out);

////////////////////////////////////////////////////////////////////////
//	SerializeMultiGridElements
///	writes the elements of a MultiGrid to a binary stream.
bool SerializeMultiGridElements(MultiGrid& mg,
								std::ostream& out);
								
////////////////////////////////////////////////////////////////////////
///	Creates multi-grid elements from a binary stream
/**
 * If you pass a pointer to a std::vector using pvVrts, pvEdges,
 * pvFaces or pvVolumes, those vectors will contain the elements
 * of the grid in the order they were read.
 * Specifying those vectors does not lead to a performance loss.
 */
bool DeserializeMultiGridElements(MultiGrid& mg, std::istream& in,
									std::vector<VertexBase*>* pvVrts = NULL,
									std::vector<EdgeBase*>* pvEdges = NULL,
									std::vector<Face*>* pvFaces = NULL,
									std::vector<Volume*>* pvVols = NULL);



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	ATTACHMENTS

////////////////////////////////////////////////////////////////////////
//	copies attached values to a binary stream.
/**
 * copies attached values of the grids elements of the given type
 * to the binary stream.
 *
 * Make sure that attachment is attached to the specified elements.
 */
template <class TElem, class TAttachment>
bool SerializeAttachment(Grid& grid, TAttachment& attachment,
						 std::ostream& out);

////////////////////////////////////////////////////////////////////////
///	copies attached values to a binary stream.
/**
 * copies attached values of the elements between iterBegin and iterEnd
 * to the binary stream.
 *
 * Make sure that attachment is attached to the specified elements.
 */
template <class TElem, class TAttachment>
bool SerializeAttachment(Grid& grid, TAttachment& attachment,
						 typename geometry_traits<TElem>::iterator iterBegin,
						 typename geometry_traits<TElem>::iterator iterEnd,
						 std::ostream& out);

////////////////////////////////////////////////////////////////////////
///	copies attached values from a binary stream
/**
 * copies values from the given binary stream to the given attachment of
 * elements between iterBegin and iterEnd.
 * If attachment was not attached to the grid, then it will be attached
 * automatically.
 */
template <class TElem, class TAttachment>
bool DeserializeAttachment(Grid& grid, TAttachment& attachment,
						 std::istream& in);

////////////////////////////////////////////////////////////////////////
///	copies attached values from a binary stream
/**
 * copies values from the given binary stream to the given attachment of
 * elements between iterBegin and iterEnd.
 * If attachment was not attached to the grid, then it will be attached
 * automatically.
 */
template <class TElem, class TAttachment>
bool DeserializeAttachment(Grid& grid, TAttachment& attachment,
						 typename geometry_traits<TElem>::iterator iterBegin,
						 typename geometry_traits<TElem>::iterator iterEnd,
						 std::istream& in);


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	SUBSET-HANDLER

////////////////////////////////////////////////////////////////////////
///	writes the subset-indices of all elements in the goc to a stream.
bool SerializeSubsetHandler(Grid& grid, ISubsetHandler& sh,
							GeometricObjectCollection goc,
							std::ostream& out);
							
////////////////////////////////////////////////////////////////////////
///	writes the subset-indices of all elements in the grid to a stream.
bool SerializeSubsetHandler(Grid& grid, ISubsetHandler& sh,
							std::ostream& out);

////////////////////////////////////////////////////////////////////////
///	assigns subset-indices to all elements in the goc from a stream.
/**	One has to be very careful that the given goc only contains
 * the elements that were passed to the serialization routine.
 * Problems could be caused by automatic element creation.
 * consider to set grid.set_option(GRIDOPT_NONE) before loading
 * the grid.
 */
bool DeserializeSubsetHandler(Grid& grid, ISubsetHandler& sh,
							GeometricObjectCollection goc,
							std::istream& in);

							
////////////////////////////////////////////////////////////////////////
///	assigns subset-indices to all elements in the grid from a stream.
/**	One has to be very careful that the given grid only contains
 * the elements that were passed to the serialization routine.
 * Problems could be caused by automatic element creation.
 * consider to set grid.set_option(GRIDOPT_NONE) before loading
 * the grid.
 */
bool DeserializeSubsetHandler(Grid& grid, ISubsetHandler& sh,
							std::istream& in);

/*
bool SerializeSelector(Grid& grid, Selector& sel, std::ostream& out);

bool DeserializeSelector(Grid& grid, Selector& sel, std::istream& in);
*/
}

////////////////////////////////
//	include implementation
#include "serialization_impl.hpp"

#endif
