#ifndef SERIALIZE_HH
#define SERIALIZE_HH

#include "serialize_core.hh"
#include "MemBuffer.hh"
#include "TypeInfo.hh"
#include "StringOp.hh"
#include "shared_ptr.hh"
#include "type_traits.hh"
#include "inline.hh"
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <sstream>
#include <cassert>
#include <memory>

typedef void* gzFile;

namespace openmsx {

class XMLElement;

// In this section, the archive classes are defined.
//
// Archives can be categorized in two ways:
//   - backing stream they use
//   - input or output (each backing stream has exactly one input and one
//     output variant)
//
// ATM these backing streams implemented:
//   - Mem
//      Stores stream in memory. Is meant to be very compact and very fast.
//      It does not support versioning (it's not possible to load this stream
//      in a newer version of openMSX). It is also not platform independent
//      (e.g. integers are stored using native platform endianess).
//      The main use case for this archive format is regular in memory
//      snapshots, for example to support replay/rewind.
//   - XML
//      Stores the stream in a XML file. These files are meant to be portable
//      to different architectures (e.g. little/big endian, 32/64 bit system).
//      There is version information in the stream, so it should be possible
//      to load streams created with older openMSX versions a newer openMSX.
//      The XML files are meant to be human readable. Having editable XML files
//      is not a design goal (e.g. simply changing a value will probably work,
//      but swapping the position of two tag or adding or removing tags can
//      easily break the stream).
//   - Text
//      This stores to stream in a flat ascii file (one item per line). This
//      format is only written as a proof-of-concept to test the design. It's
//      not meant to be used in practice.
//
// Archive code is heavily templatized. It uses the CRTP (curiously recuring
// template pattern ; a base class templatized on it's derived class). To
// implement static polymorphism. This means there is practically no run-time
// overhead of using this mechansim compared to 6 seperatly handcoded functions
// (Mem/XML/Text x input/output).
// TODO At least in theory, still need to verify this in practice.
//      Though my experience is that gcc is generally very good in this.

template<typename Derived> class ArchiveBase
{
public:
	/** Is this archive a loader or a saver.
	bool isLoader() const;*/

	/** Serialize the base class of this classtype.
	 * Should preferably be called as the first statement in the
	 * implementation of a serialize() method of a class type.
	 * See also serializeInlinedBase() below.
	 */
	template<typename Base, typename T>
	void serializeBase(T& t)
	{
		const char* tag = BaseClassName<Base>::getName();
		self().serialize(tag, static_cast<Base&>(t));
	}

	/** Serialize the base class of this classtype.
	 * Should preferably be called as the first statement in the
	 * implementation of a serialize() method of a class type.
	 * See also serializeBase() above.
	 *
	 * The differece between serializeBase() and serializeInlinedBase()
	 * is only relevant for versioned archives (see needVersion(), e.g.
	 * XML archives). In XML archives serializeBase() will put the base
	 * class in a new subtag, serializeInlinedBase() puts the members
	 * of the base class (inline) in the current tag. The advantage
	 * of serializeBase() is that the base class can be versioned
	 * seperatly from the subclass. The disadvantage is that it exposes
	 * an internal implementation detail in the XML file, and thus makes
	 * it harder to for example change the class hierarchy or move
	 * members from base to subclass or vice-versa.
	 */
	template<typename Base, typename T>
	void serializeInlinedBase(T& t, unsigned version)
	{
		::openmsx::serialize(self(), static_cast<Base&>(t), version);
	}

	// Each concrete archive class also has the following methods:
	// Because of the implementation with static polymorphism, this
	// interface is not explictly visible in the base class.
	//
	//
	// template<typename T> void serializeWithID(const char* tag, const T& t, ...)
	//
	//   This is _the_most_important_ method of the serialization
	//   framework. Depending on the concrete archive type (loader/saver)
	//   this method will load or save the given type 't'. In case of an XML
	//   archive the 'tag' parameter will be used as tagname.
	//
	//   At the end there are still a number of optional parameters (in the
	//   current implementation there can be between 0 and 3, but can be
	//   extened when needed). These are 'global' constructor parameters,
	//   constructor parameters that are not stored in the stream, but that
	//   are needed to reconstruct the object (for example can be references
	//   to structures that were already stored in the stream). So these
	//   parameters are only actually used while loading.
	//   TODO document this in more detail in some section where the
	//        (polymorphic) constructors are also described.
	//
	//
	// void serialize_blob(const char* tag, const void* data, unsigned len)
	//
	//   Serialize the given data as a binary blob.
	//   This cannot be part of the serialize() method above because we
	//   cannot know whether a byte-array should be serialized as a blob
	//   or as a collection of bytes (IOW we cannot decide it based on the
	//   type).
	//
	//
	// template<typename T> void serialize(const char* tag, const T& t)
	//
	//   This is much like the serializeWithID() method above, but it doesn't
	//   store an ID with this element. This means that it's not possible,
	//   later on in the stream, to refer to this element. For many elements
	//   you know this will not happen. This method results in a slightly
	//   more compact stream.
	//
	//   Note that for primitive types we already don't store an ID, because
	//   pointers to primitive types are not supported (at least not ATM).
	//
	//
	// template<typename T> void serializePointerID(const char* tag, const T& t)
	//
	//   Serialize a pointer by storing the ID of the object it points to.
	//   This only works if the object was already serialized. The only
	//   reason to use this method instead of the more general serialize()
	//   method is that this one does not instantiate the object
	//   construction code. (So in some cases you can avoid having to
	//   provide specializations of SerializeConstructorArgs.)
	//
	//
	// template<typename T> void serializePolymorphic(const char* tag, const T& t)
	//
	//   Serialize a value-type whose concrete type is not yet known at
	//   compile-time (polymorphic pointers are already handled by the
	//   generic serialize() method).
	//
	//   The difference between pointer and value-types is that for
	//   pointers, the de-serialize code also needs to construct the
	//   object, while for value-types, the object (with the correct
	//   concrete type) is already constructed, it only needs to be
	//   initialized.

/*internal*/
	// These must be public for technical reasons, but they should only
	// be used by the serialization framework.

	/** Does this archive store version information. */
	bool needVersion() const { return true; }

	/** Does this archive store enums as strings.
	 * See also struct serialize_as_enum.
	 */
	bool translateEnumToString() const { return false; }

	/** Load/store an attribute from/in the archive.
	 * Depending on the underlying concrete stream, attributes are either
	 * stored like XML attributes or as regular values. Because of this
	 * (and thus unlike XML attributes) the order of attributes matter. It
	 * also matters whether an attribute is present or not.
	 */
	template<typename T> void attribute(const char* name, T& t)
	{
		self().serialize(name, t);
	}
	void attribute(const char* name, const char* value);

	/** Some archives (like XML archives) can store optional attributes.
	 * This method indicates whether that's the case or not.
	 * This can be used to for example in XML files don't store attributes
	 * with default values (thus to make the XML look prettier).
	 */
	bool canHaveOptionalAttributes() const { return false; }

	/** Check the presence of a (optional) attribute.
	 * It's only allowed to call this method on archives that can have
	 * optional attributes.
	 */
	bool hasAttribute(const std::string& /*name*/)
	{
		assert(false); return false;
	}

	/** Some archives (like XML archives) can count the number of subtags
	 * that belong to the current tag. This method indicates whether that's
	 * the case for this archive or not.
	 * This can for example be used to make the XML files look prettier in
	 * case of serialization of collections: in that case we don't need to
	 * explictly store the size of the collection, it can be derived from
	 * the number of subtags.
	 */
	bool canCountChildren() const { return false; }

	/** Count the number of child tags.
	 * It's only allowed to call this method on archives that have support
	 * for this operation.
	 */
	int countChildren() const { assert(false); return -1; }

	/** Indicate begin of a tag.
	 * Only XML archives use this, other archives ignore it.
	 * XML saver uses it as a name for the current tag, it doesn't
	 * interpret the name in any way.
	 * XML loader uses it only as a check: it checks whether the current
	 * tag name matches the given name. So we will NOT search the tag
	 * with the given name, the tags have to be in the correct order.
	 */
	void beginTag(const char* /*tag*/)
	{
		// nothing
	}
	/** Indicate begin of a tag.
	 * Only XML archives use this, other archives ignore it.
	 * Both XML loader and saver only use the given tag name to do some
	 * internal checks (with checks disabled, the tag parameter has no
	 * influence at all on loading or saving of the stream).
	 */
	void endTag(const char* /*tag*/)
	{
		// nothing
	}

	// These (internal) methods should be implemented in the concrete
	// archive classes.
	//
	// template<typename T> void save(const T& t)
	//
	//   Should only be implemented for OuputArchives. Is called to
	//   store primitive types in the stream. In the end all structures
	//   are broken down to primitive types, so all data that ends up
	//   in the stream passes via this method (ok, depending on how
	//   attribute() and serialize_blob() is implemented, that data may
	//   not pass via save()).
	//
	//   Often this method will be overloaded to handle certain types in a
	//   specific way.
	//
	//
	// template<typename T> void load(T& t)
	//
	//   Should only be implemented for InputArchives. This is similar (but
	//   opposite) to the save() method above. Loading of primitive types
	//   is done via this method.

	// void beginSection()
	// void endSection()
	// void skipSection(bool skip)
	//   The methods beginSection() and endSection() can only be used in
	//   output archives. These mark the location of a section that can
	//   later be skipped during loading.
	//   The method skipSection() can only be used in input archives. It
	//   optionally skips a section that was marked during saving.
	//   For every beginSection() call in the output, there must be a
	//   corresponding skipSection() call in the input (even if you don't
	//   actually want to skip the section).

protected:
	/** Returns a reference to the most derived class.
	 * Helper function to implement static polymorphism.
	 */
	inline Derived& self()
	{
		return static_cast<Derived&>(*this);
	}
};

// The part of OutputArchiveBase that doesn't depend on the template parameter
class OutputArchiveBase2
{
public:
	inline bool isLoader() const { return false; }

	void skipSection(bool /*skip*/)
	{
		assert(false);
	}

/*internal*/
	#ifdef linux
	// This routine is not portable, for example it breaks in
	// windows (mingw) because there the location of the stack
	// is _below_ the heap.
	// But this is anyway only used to check assertions. So for now
	// only do that in linux.
	static NEVER_INLINE bool addressOnStack(const void* p)
	{
		// This is not portable, it assumes:
		//  - stack grows downwards
		//  - heap is at lower address than stack
		// Also in c++ comparison between pointers is only defined when
		// the two pointers point to objects in the same array.
		int dummy;
		return &dummy < p;
	}
	#endif

	// Generate a new ID for the given pointer and store this association
	// for later (see getId()).
	template<typename T> unsigned generateId(const T* p)
	{
		// For composed structures, for example
		//   struct A { ... };
		//   struct B { A a; ... };
		// The pointer to the outer and inner structure can be the
		// same while we still want a different ID to refer to these
		// two. That's why we use a std::pair<void*, TypeInfo> as key
		// in the map.
		// For polymorphic types you do sometimes use a base pointer
		// to refer to a subtype. So there we only use the pointer
		// value as key in the map.
		if (is_polymorphic<T>::value) {
			return generateID1(p);
		} else {
			return generateID2(p, typeid(T));
		}
	}

	template<typename T> unsigned getId(const T* p)
	{
		if (is_polymorphic<T>::value) {
			return getID1(p);
		} else {
			return getID2(p, typeid(T));
		}
	}

protected:
	OutputArchiveBase2();

private:
	unsigned generateID1(const void* p);
	unsigned generateID2(const void* p, const std::type_info& typeInfo);
	unsigned getID1(const void* p);
	unsigned getID2(const void* p, const std::type_info& typeInfo);

	typedef std::pair<const void*, TypeInfo> IdKey;
	typedef std::map<IdKey, unsigned> IdMap;
	typedef std::map<const void*, unsigned> PolyIdMap;
	IdMap idMap;
	PolyIdMap polyIdMap;
	unsigned lastId;
};

template<typename Derived>
class OutputArchiveBase : public ArchiveBase<Derived>, public OutputArchiveBase2
{
public:
	// Main saver method. Heavy lifting is done in the Saver class.
	template<typename T> void serializeWithID(const char* tag, const T& t)
	{
		this->self().beginTag(tag);
		Saver<T> saver;
		saver(this->self(), t, true);
		this->self().endTag(tag);
	}
	// 3 methods below implement 'global constructor arguments'. Though
	// the saver archives completly ignore those extra parameters. We
	// anyway need to provide them because the same (templatized) code
	// path is used both for saving and loading.
	template<typename T, typename T1>
	void serializeWithID(const char* tag, const T& t, T1 /*t1*/)
	{
		serializeWithID(tag, t);
	}
	template<typename T, typename T1, typename T2>
	void serializeWithID(const char* tag, const T& t, T1 /*t1*/, T2 /*t2*/)
	{
		serializeWithID(tag, t);
	}
	template<typename T, typename T1, typename T2, typename T3>
	void serializeWithID(const char* tag, const T& t, T1 /*t1*/, T2 /*t2*/, T3 /*t3*/)
	{
		serializeWithID(tag, t);
	}

	// Default implementation is to base64-encode the blob and serialize
	// the resulting string. But memory archives will memcpy the blob.
	void serialize_blob(const char* tag, const void* data, unsigned len);

	template<typename T> void serialize(const char* tag, const T& t)
	{
		this->self().beginTag(tag);
		Saver<T> saver;
		saver(this->self(), t, false);
		this->self().endTag(tag);
	}
	template<typename T> void serializePointerID(const char* tag, const T& t)
	{
		this->self().beginTag(tag);
		IDSaver<T> saver;
		saver(this->self(), t);
		this->self().endTag(tag);
	}
	template<typename T> void serializePolymorphic(const char* tag, const T& t)
	{
		STATIC_ASSERT(is_polymorphic<T>::value);
		PolymorphicSaverRegistry<Derived>::save(tag, this->self(), t);
	}

protected:
	OutputArchiveBase() {}
};


// Part of InputArchiveBase that doesn't depend on the template parameter
class InputArchiveBase2
{
public:
	inline bool isLoader() const { return true; }

	void beginSection()
	{
		assert(false);
	}
	void endSection()
	{
		assert(false);
	}

/*internal*/
	void* getPointer(unsigned id);
	void addPointer(unsigned id, const void* p);

	template<typename T> void resetSharedPtr(shared_ptr<T>& s, T* r)
	{
		if (r == NULL) {
			s.reset();
			return;
		}
		SharedPtrMap::const_iterator it = sharedPtrMap.find(r);
		if (it == sharedPtrMap.end()) {
			s.reset(r);
			sharedPtrMap[r] = &s;
			//sharedPtrMap[r] = s;
		} else {
			shared_ptr<T>* s2 = static_cast<shared_ptr<T>*>(it->second);
			s = *s2;
			//s = std::tr1::static_pointer_cast<T>(it->second);
		}
	}

protected:
	InputArchiveBase2() {}

private:
	typedef std::map<unsigned, void*> IdMap;
	IdMap idMap;

	typedef std::map<void*, void*> SharedPtrMap;
	//typedef std::map<void*, std::tr1::shared_ptr<void> > SharedPtrMap;
	SharedPtrMap sharedPtrMap;
};

template<typename Derived>
class InputArchiveBase : public ArchiveBase<Derived>, public InputArchiveBase2
{
public:
	template<typename T>
	void serializeWithID(const char* tag, T& t)
	{
		doSerialize(tag, t, make_tuple());
	}
	template<typename T, typename T1>
	void serializeWithID(const char* tag, T& t, T1 t1)
	{
		doSerialize(tag, t, make_tuple(t1));
	}
	template<typename T, typename T1, typename T2>
	void serializeWithID(const char* tag, T& t, T1 t1, T2 t2)
	{
		doSerialize(tag, t, make_tuple(t1, t2));
	}
	template<typename T, typename T1, typename T2, typename T3>
	void serializeWithID(const char* tag, T& t, T1 t1, T2 t2, T3 t3)
	{
		doSerialize(tag, t, make_tuple(t1, t2, t3));
	}
	void serialize_blob(const char* tag, void* data, unsigned len);

	template<typename T>
	void serialize(const char* tag, T& t)
	{
		this->self().beginTag(tag);
		typedef typename remove_const<T>::type TNC;
		TNC& tnc = const_cast<TNC&>(t);
		Loader<TNC> loader;
		loader(this->self(), tnc, make_tuple(), -1); // don't load id
		this->self().endTag(tag);
	}
	template<typename T> void serializePointerID(const char* tag, const T& t)
	{
		this->self().beginTag(tag);
		typedef typename remove_const<T>::type TNC;
		TNC& tnc = const_cast<TNC&>(t);
		IDLoader<TNC> loader;
		loader(this->self(), tnc);
		this->self().endTag(tag);
	}
	template<typename T> void serializePolymorphic(const char* tag, T& t)
	{
		STATIC_ASSERT(is_polymorphic<T>::value);
		PolymorphicInitializerRegistry<Derived>::init(tag, this->self(), &t);
	}

/*internal*/
	// Actual loader method. Heavy lifting is done in the Loader class.
	template<typename T, typename TUPLE>
	void doSerialize(const char* tag, T& t, TUPLE args, int id = 0)
	{
		this->self().beginTag(tag);
		typedef typename remove_const<T>::type TNC;
		TNC& tnc = const_cast<TNC&>(t);
		Loader<TNC> loader;
		loader(this->self(), tnc, args, id);
		this->self().endTag(tag);
	}

protected:
	InputArchiveBase() {}
};

////
/*
class TextOutputArchive : public OutputArchiveBase<TextOutputArchive>
{
public:
	TextOutputArchive(const std::string& filename)
	{
		FileOperations::openofstream(os, filename);
	}

	template<typename T> void save(const T& t)
	{
		if (is_floating<T>::value) {
			os << std::setprecision(std::numeric_limits<T>::digits10 + 2);
		}
		os << t << '\n';
	}
	void serialize_blob(const char* tag, const void* data, unsigned len)
	{
		std::string tmp = Base64::encode(data, len, false); // no newlines
		serialize(tag, tmp);
	}

private:
	std::ofstream os;
};

class TextInputArchive : public InputArchiveBase<TextInputArchive>
{
public:
	TextInputArchive(const std::string& filename)
		: is(filename.c_str())
	{
	}

	template<typename T> void load(T& t)
	{
		std::string s;
		getline(is, s);
		std::istringstream ss(s);
		ss >> t;
	}
	void load(std::string& t)
	{
		// !!! this doesn't handle strings with !!!
		// !!! embedded newlines correctly      !!!
		getline(is, t);
	}

private:
	std::ifstream is;
};
*/
////

class MemOutputArchive : public OutputArchiveBase<MemOutputArchive>
{
public:
	MemOutputArchive()
	{
	}

	~MemOutputArchive()
	{
		assert(openSections.empty());
	}

	bool needVersion() const { return false; }

	template <typename T> void save(const T& t)
	{
		put(&t, sizeof(t));
	}
	void save(const std::string& s);
	void serialize_blob(const char*, const void* data, unsigned len)
	{
		put(data, len);
	}

	void beginSection()
	{
		unsigned skip = 0; // filled in later
		save(skip);
		unsigned beginPos = buffer.getPosition();
		openSections.push_back(beginPos);
	}
	void endSection()
	{
		assert(!openSections.empty());
		unsigned endPos   = buffer.getPosition();
		unsigned beginPos = openSections.back();
		openSections.pop_back();
		unsigned skip = endPos - beginPos;
		buffer.insertAt(beginPos - sizeof(unsigned),
		                &skip, sizeof(skip));
	}

	OutputBuffer& stealBuffer() { return buffer; }

private:
	void put(const void* data, unsigned len)
	{
		if (len) {
			buffer.insert(data, len);
		}
	}

	OutputBuffer buffer;
	std::vector<unsigned> openSections;
};

class MemInputArchive : public InputArchiveBase<MemInputArchive>
{
public:
	MemInputArchive(const MemBuffer& mem)
		: buffer(mem.getData(), mem.getLength())
	{
	}

	bool needVersion() const { return false; }

	template<typename T> void load(T& t)
	{
		get(&t, sizeof(t));
	}
	void load(std::string& s);
	void serialize_blob(const char*, void* data, unsigned len)
	{
		get(data, len);
	}

	void skipSection(bool skip)
	{
		unsigned num;
		load(num);
		if (skip) {
			buffer.skip(num);
		}
	}

private:
	void get(void* data, unsigned len)
	{
		if (len) {
			buffer.read(data, len);
		}
	}

	InputBuffer buffer;
	unsigned pos;
};

////

class XmlOutputArchive : public OutputArchiveBase<XmlOutputArchive>
{
public:
	XmlOutputArchive(const std::string& filename);
	~XmlOutputArchive();

	template <typename T> void saveImpl(const T& t)
	{
		// TODO make sure floating point is printed with enough digits
		//      maybe print as hex?
		save(StringOp::toString(t));
	}
	template <typename T> void save(const T& t)
	{
		saveImpl(t);
	}
	void save(const std::string& str);
	void save(bool b);
	void save(unsigned char b);
	void save(signed char c);
	void save(int i);                  // these 3 are not strictly needed
	void save(unsigned u);             // but having them non-inline
	void save(unsigned long long ull); // saves quite a bit of code

	void beginSection() { /*nothing*/ }
	void endSection()   { /*nothing*/ }

//internal:
	inline bool translateEnumToString() const { return true; }
	inline bool canHaveOptionalAttributes() const { return true; }
	inline bool canCountChildren() const { return true; }

	void beginTag(const char* tag);
	void endTag(const char* tag);

	template<typename T> void attribute(const std::string& name, const T& t)
	{
		attribute(name, StringOp::toString(t));
	}
	void attribute(const std::string& name, const std::string& str);

private:
	gzFile file;
	XMLElement* current;
};

class XmlInputArchive : public InputArchiveBase<XmlInputArchive>
{
public:
	XmlInputArchive(const std::string& filename);

	template<typename T> void loadImpl(T& t)
	{
		std::string str;
		load(str);
		std::istringstream is(str);
		is >> t;
	}
	template<typename T> void load(T& t)
	{
		loadImpl(t);
	}
	void load(std::string& t);
	void load(bool& b);
	void load(unsigned char& b);
	void load(signed char& c);
	void load(int& i);                  // these 3 are not strictly needed
	void load(unsigned& u);             // but having them non-inline
	void load(unsigned long long& ull); // saves quite a bit of code

	void skipSection(bool /*skip*/) { /*nothing*/ }

//internal:
	inline bool translateEnumToString() const { return true; }
	inline bool canHaveOptionalAttributes() const { return true; }
	inline bool canCountChildren() const { return true; }

	void beginTag(const char* tag);
	void endTag(const char* tag);

	template<typename T> void attribute(const std::string& name, T& t)
	{
		std::string str;
		attribute(name, str);
		std::istringstream is(str);
		is >> t;
	}
	void attribute(const std::string& name, std::string& t);

	bool hasAttribute(const std::string& name);
	int countChildren() const;

private:
	void init(const XMLElement* e);

	std::auto_ptr<XMLElement> elem;
	std::vector<const XMLElement*> elems;
	unsigned pos;
};

/*#define INSTANTIATE_SERIALIZE_METHODS(CLASS) \
template void CLASS::serialize(TextInputArchive&,  unsigned); \
template void CLASS::serialize(TextOutputArchive&, unsigned); \
template void CLASS::serialize(MemInputArchive&,   unsigned); \
template void CLASS::serialize(MemOutputArchive&,  unsigned); \
template void CLASS::serialize(XmlInputArchive&,   unsigned); \
template void CLASS::serialize(XmlOutputArchive&,  unsigned); */
#define INSTANTIATE_SERIALIZE_METHODS(CLASS) \
template void CLASS::serialize(MemInputArchive&,   unsigned); \
template void CLASS::serialize(MemOutputArchive&,  unsigned); \
template void CLASS::serialize(XmlInputArchive&,   unsigned); \
template void CLASS::serialize(XmlOutputArchive&,  unsigned);

} // namespace openmsx

#endif
