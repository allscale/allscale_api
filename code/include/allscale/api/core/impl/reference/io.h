#pragma once

#include <string>
#include <mutex>
#include <fstream>

#include "allscale/utils/assert.h"
#include "allscale/utils/serializer.h"


namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	/**
	 * Supported IO modes.
	 */
	enum class Mode {
		Text, Binary
	};

	/**
	 * The kind of handle to reference entities within an IO manager.
	 */
	struct Entry {
		std::size_t id;
		bool operator<(const Entry& other) const { return id < other.id; }
	};

	/**
	 * A common base class for Input and Output Streams.
	 */
	class IOStream {
	protected:

		Entry entry;

		std::mutex operation_lock;

		IOStream(const Entry& entry) : entry(entry) {}

		IOStream(IOStream&& other)
			: entry(other.entry) {}

	public:

		Entry getEntry() const {
			return entry;
		}

	};

	/**
	 * A stream to load data in the form of a stream of entries.
	 */
	class InputStream : public IOStream {

		template<typename Factory>
		friend class IOManager;

		struct IStreamWrapper {
			std::istream& in;
			IStreamWrapper(std::istream& in) : in(in) {}
			template<typename T>
			IStreamWrapper& operator>>(T& value) {
				in >> value;
				return *this;
			}
			template<typename T>
			T read() {
				T value;
				in.read((char*)&value, sizeof(T));
				return value;
			}
			template<typename T>
			IStreamWrapper& read(T& res) {
				in.read((char*)&res, sizeof(T));
				return *this;
			}
		};

		IStreamWrapper in;

		InputStream(const Entry& entry, std::istream& in)
			: IOStream(entry), in(in) {}

	public:

		InputStream(InputStream&& other)
			: IOStream(std::move(other)), in(other.in) {}

		template<typename Body>
		void atomic(const Body& body) {
			// protect output by locking it
			std::lock_guard<std::mutex> lease(operation_lock);

			// let the body read it's information
			body(in);

			// free the lock - automatically
		}

		template<typename T>
		void operator>>(T& value) {
			atomic([&](IStreamWrapper& in) { in >> value; });
		}

		template<typename T>
		T read() {
			T res;
			atomic([&](IStreamWrapper& in) {
				res = in.read<T>();
			});
			return res;
		}

		operator bool() const {
			return (bool)in.in;
		}

		static InputStream& load(utils::Archive&) {
			assert_not_implemented();
			return *static_cast<InputStream*>(nullptr);
		}

		void store(utils::Archive&) const {
			assert_not_implemented();
		}
	};

	/**
	 * A stream to store data in the form of a stream of entries.
	 */
	class OutputStream : public IOStream {

		template<typename Factory>
		friend class IOManager;

		struct OStreamWrapper {
			std::ostream& out;
			OStreamWrapper(std::ostream& out) : out(out) {}
			template<typename T>
			OStreamWrapper& operator<<(const T& value) {
				out << value;
				return *this;
			}
			template<typename T>
			OStreamWrapper& write(const T& value) {
				out.write((char*)&value, sizeof(T));
				return *this;
			}
		};

		OStreamWrapper out;

		OutputStream(const Entry& entry, std::ostream& out)
			: IOStream(entry), out(out) {}

	public:

		OutputStream(OutputStream&& other)
			: IOStream(std::move(other)), out(other.out) {}

		template<typename Body>
		void atomic(const Body& body) {
			// protect output by locking it
			std::lock_guard<std::mutex> lease(operation_lock);

			// let the body write it's information
			body(out);

			// free the lock - automatically
		}

		template<typename T>
		void operator<<(const T& value) {
			atomic([&](OStreamWrapper& out) {
				out << value;
			});
		}

		template<typename T>
		void write(const T& value) {
			atomic([&](OStreamWrapper& out) {
				out.write(value);
			});
		}

		operator bool() const {
			return (bool)out.out;
		}

		static OutputStream& load(utils::Archive&) {
			assert_not_implemented();
			return *static_cast<OutputStream*>(nullptr);
		}

		void store(utils::Archive&) const {
			assert_not_implemented();
		}
	};

	/**
	 * An IO manager, as the central dispatcher for IO operations.
	 */
	template<typename StorageManager>
	class IOManager {

		/**
		 * The underlying store.
		 */
		StorageManager store;

		/**
		 * The central register of all open output streams.
		 */
		std::map<Entry,InputStream> inputStreams;

		/**
		 * The central register of all open output streams.
		 */
		std::map<Entry,OutputStream> outputStreams;

	public:

		~IOManager() {
			// close and destroy all input streams
			for(auto& cur : inputStreams) {
				closeStream(cur.second);
			}
			// close and destroy all output streams
			for(auto& cur : outputStreams) {
				closeStream(cur.second);
			}
		}

		/**
		 * Creates a new entry with the given name in the underlying storage system.
		 *
		 * @param name the name of the entry (e.g. file)
		 * @param mode whether it is a binary or text file
		 * @return a entry ID referencing the newly created resource
		 */
		Entry createEntry(const std::string& name, Mode mode = Mode::Text) {
			return store.createEntry(name, mode);
		}

		/**
		 * Register a new output stream with the given name within the system.
		 * The call will create the underlying file and prepare output operations.
		 *
		 *  NOTE: this method is not thread save!
		 *
		 * @param name the name of the stream to be opened -- nothing happens if already opened
		 */
		InputStream& openInputStream(Entry entry) {

			// check for present
			auto pos = inputStreams.find(entry);
			if (pos != inputStreams.end()) return pos->second;

			// create new input stream
			InputStream res(entry, *store.createInputStream(entry));

			// register stream
			inputStreams.emplace(entry, std::move(res));

			// return result
			return getInputStream(entry);
		}

		/**
		 * Register a new output stream with the given name within the system.
		 * The call will create the underlying file and prepare output operations.
		 *
		 *  NOTE: this method is not thread save!
		 *
		 * @param name the name of the stream to be opened -- nothing happens if already opened
		 */
		OutputStream& openOutputStream(Entry entry) {

			// check for present
			auto pos = outputStreams.find(entry);
			if (pos != outputStreams.end()) return pos->second;

			// create new input stream
			OutputStream res(entry, *store.createOutputStream(entry));

			// register stream
			outputStreams.emplace(entry, std::move(res));

			// return result
			return getOutputStream(entry);
		}

		/**
		 * Closes the stream with the given name.
		 */
		void closeInputStream(Entry entry) {
			// get the stream
			auto pos = inputStreams.find(entry);
			if (pos == inputStreams.end()) return;

			// close the stream
			closeStream(pos->second);

			// erase the entry
			inputStreams.erase(pos);
		}

		/**
		 * Closes the stream with the given name.
		 */
		void closeOutputStream(Entry entry) {
			// get the stream
			auto pos = outputStreams.find(entry);
			if (pos == outputStreams.end()) return;

			// close the stream
			closeStream(pos->second);

			// erase the entry
			outputStreams.erase(pos);
		}

		/**
		 * Closes the given stream.
		 */
		void close(const InputStream& in) {
			closeInputStream(in.getEntry());
		}

		/**
		 * Closes the given stream.
		 */
		void close(const OutputStream& out) {
			closeOutputStream(out.getEntry());
		}

		/**
		 * Obtains an input stream to read data from a storage entry.
		 * The storage entry is maintained by the manager and the provided output stream
		 * is only valid within the current thread.
		 *
		 * @param the name of the storage entry to be targeted -- must be open
		 * @return a stream to append data to
		 */
		InputStream& getInputStream(Entry entry) {
			assert(inputStreams.find(entry) != inputStreams.end());
			return inputStreams.find(entry)->second;
		}

		/**
		 * Obtains an input stream to read data from a storage entry.
		 * The storage entry is maintained by the manager and the provided input stream
		 * is only valid within the current thread.
		 *
		 * @param the name of the storage entry to be targeted -- must be open
		 * @return a stream to append data to
		 */
		OutputStream& getOutputStream(Entry entry) {
			assert(outputStreams.find(entry) != outputStreams.end());
			return outputStreams.find(entry)->second;
		}

		/**
		 * Determines whether the given entry exists.
		 */
		bool exists(Entry entry) const {
			return store.exists(entry);
		}

		/**
		 * Deletes the entry with the given name.
		 */
		void remove(Entry entry) {
			store.remove(entry);
		}

	private:

		/**
		 * Closes the given input stream.
		 */
		void closeStream(InputStream& in) {
			// closes the stream
			store.close(in.in.in);
		}

		/**
		 * Closes the given output stream.
		 */
		void closeStream(OutputStream& out) {
			// closes the stream
			store.close(out.out.out);
		}

	};



	// ----------------------------------------------------------------------
	//				    for in-memory buffer operations
	// ----------------------------------------------------------------------


	struct BufferStreamFactory {

		struct Buffer {
			std::string name;
			Mode mode;
			std::stringstream* stream;
		};

		std::size_t counter = 0;

		std::map<Entry, Buffer> buffers;

		~BufferStreamFactory() {
			for(const auto& cur : buffers) delete cur.second.stream;
		}

		Entry createEntry(const std::string& name, Mode mode) {
			// check for present entry
			for(const auto& cur : buffers) {
				if (cur.second.name == name) {
					return cur.first;
				}
			}

			// create a new entry
			Entry id{counter++};
			Buffer& entry = buffers[id];
			entry.name = name;
			entry.mode = mode;
			entry.stream = nullptr;
			return id;
		}

		std::istream* createInputStream(Entry entry) {

			// search for entry
			auto pos = buffers.find(entry);
			if (pos == buffers.end()) {
				assert(false && "Unable to create input stream to unknown entity!");
				return nullptr;
			}


			// reuse current stream content
			std::stringstream* old = pos->second.stream;
			std::stringstream* res = (pos->second.mode == Mode::Binary) ?
					new std::stringstream((old) ? old->str() : std::basic_string<char>(), std::ios_base::in | std::ios_base::binary ) :
					new std::stringstream((old) ? old->str() : std::basic_string<char>(), std::ios_base::in );
			delete old;
			pos->second.stream = res;
			return res;
		}

		std::ostream* createOutputStream(Entry entry) {

			// search for entry
			auto pos = buffers.find(entry);
			if (pos == buffers.end()) {
				assert(false && "Unable to create output stream to unknown entity!");
				return nullptr;
			}

			// reuse current stream content
			std::stringstream* old = pos->second.stream;
			std::stringstream* res = (pos->second.mode == Mode::Binary) ?
					new std::stringstream((old) ? old->str() : std::basic_string<char>(), std::ios_base::out | std::ios_base::binary ) :
					new std::stringstream((old) ? old->str() : std::basic_string<char>(), std::ios_base::out );
			delete old;
			pos->second.stream = res;
			return res;
		}

		void close(std::istream&) {
			// nothing to do
		}

		void close(std::ostream&) {
			// nothing to do
		}

		bool exists(Entry entry) const {
			return buffers.find(entry) != buffers.end();
		}

		void remove(Entry entry) {
			auto pos = buffers.find(entry);
			if (pos == buffers.end()) return;
			delete pos->second.stream;
			buffers.erase(pos);
		}
	};

	class BufferIOManager : public IOManager<BufferStreamFactory> {

	};


	// ----------------------------------------------------------------------
	//					  	  for file IO
	// ----------------------------------------------------------------------

	struct FileStreamFactory {

		struct File {
			std::string name;
			Mode mode;
		};

		std::vector<File> files;

		Entry createEntry(const std::string& name, Mode mode) {
			// check for present entry
			for(std::size_t i=0; i < files.size(); ++i) {
				if (files[i].name == name) return Entry{i};
			}

			// create a new entry
			Entry id{files.size()};
			files.push_back(File{name,mode});
			return id;
		}

		std::istream* createInputStream(Entry entry) {

			// check valid entry id
			if (entry.id >= files.size()) {
				assert(false && "Unable to create output stream to unknown entity!");
				return nullptr;
			}

			// create a matching file stream
			const File& file = files[entry.id];
			return (file.mode == Mode::Binary) ?
				new std::fstream(file.name,std::ios_base::in | std::ios_base::binary) :
				new std::fstream(file.name,std::ios_base::in);
		}

		std::ostream* createOutputStream(Entry entry) {

			// check valid entry id
			if (entry.id >= files.size()) {
				assert(false && "Unable to create output stream to unknown entity!");
				return nullptr;
			}

			// create a matching file stream
			const File& file = files[entry.id];
			return (file.mode == Mode::Binary) ?
				new std::fstream(file.name,std::ios_base::out | std::ios_base::binary) :
				new std::fstream(file.name,std::ios_base::out);
		}

		void close(std::istream& stream) {
			delete &stream;
		}

		void close(std::ostream& stream) {
			delete &stream;
		}

		bool exists(Entry entry) const {
			if (entry.id >= files.size()) return false;
			struct stat buffer;
			return stat(files[entry.id].name.c_str(), &buffer) == 0;
		}

		void remove(Entry entry) {
			if (entry.id >= files.size()) return;
			std::remove(files[entry.id].name.c_str());
		}

	};

	class FileIOManager : public IOManager<FileStreamFactory> {
		FileIOManager() {};
	public:
		static FileIOManager& getInstance() {
			static FileIOManager manager;
			return manager;
		}
	};


	/**
	 * Obtains access to the singleton instance of the File IO manager.
	 */
	inline static FileIOManager& getFileIOManager() {
		return FileIOManager::getInstance();
	}


} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
