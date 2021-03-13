/*
 * sharedmem.h
 *
 *  Created on: 30.10.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>	/* For mode constants */
#include <fcntl.h>		/* For O_* constants */
#include <string>
#include <memory>
#include "classes.h"
#include "templates.h"
#include "exception.h"
#include "random.h"

#ifndef SHM_DEFAULT_ACL
#  define SHM_DEFAULT_ACL (S_IRWXU)
#endif

namespace util {

template<typename T>
class TDataBuffer {
protected:
	typedef T buffer_t;

	virtual buffer_t* create(size_t size) {
		return new buffer_t[size];
	}
	virtual void destroy() {
		if (nil != m_buffer)
			delete[] m_buffer;
	}

	void init(const size_t& size, const size_t& capacity = 0) {
		m_owner = true;
		m_buffer = nil;
		d_buffer = nil;
		m_size = size;
		m_ordinal = sizeof(buffer_t);
		if (size > capacity) m_capacity = estimate(size);
		else m_capacity = capacity;
		expand(false);
	}

	void reset() {
    	if (nil != d_buffer)
    		delete[] d_buffer;
    	d_buffer = nil;
		m_buffer = nil;
		m_capacity = 0;
		m_size = 0;
	}

	void rewind() {
		if (m_size > 0 && m_buffer != nil) {
			m_buffer[0] = (buffer_t)0;
		}
		m_size = 0;
	}

private:
	bool m_owner;
	buffer_t* m_buffer;
	std::size_t m_size;
	std::size_t m_capacity;
	std::size_t m_ordinal;
	mutable buffer_t* d_buffer;

	inline size_t offset(const size_t& position) const {
		return (position * m_ordinal);
	}
	inline size_t estimate(const size_t& value) const {
		// Assume capacity = 110% of value or size + 1 for terminator!
		return value * 11 / 10 + 1;
	}
	void terminate() {
		if (m_buffer != nil && m_size > 0 && m_capacity > m_size) {
			m_buffer[m_size] = (buffer_t)0;
		}
	}
	void copy(buffer_t * dst, const buffer_t *const src, std::size_t size) {
		if (size > 0)
			::memcpy(dst, src, offset(size));
	}
	void expand(bool retain) {
		if (m_capacity) {
			buffer_t* p = create(m_capacity);
			if (m_buffer != nil) {
				if (retain)
					copy(p, m_buffer, m_size);
				destroy();
			}
			m_buffer = p;
			terminate();
		}
	}

public:
	// Properties and getters
	bool empty() const { return (m_buffer == nil || m_size <= 0); }
	const size_t size() const { return m_size; }
	const size_t capacity() const { return m_capacity; }
	const size_t ordinal() const { return m_ordinal; }

	buffer_t* data() const { return m_buffer; }
	char* c_str() const { return (char*)m_buffer; }

	bool validIndex(const size_t index) const { return (index >= 0 && index < size()); }

	// Methods
	void resize(std::size_t size, bool retain = true) {
		if (size > m_size) {
			if (size > m_capacity) {
				m_capacity = estimate(size);
				expand(retain);
			}
		}
		m_size = assigned(m_buffer) ? size : (size_t)0;
	}
	void reserve(std::size_t capacity, bool retain = true) {
		if (capacity > m_capacity) {
			m_capacity = capacity;
			expand(retain);
		}
	}
	void append(const TDataBuffer<buffer_t>& buffer) {
		append(buffer.data(), buffer.size());
	}
	void append(const void *const data, std::size_t size) {
		if (data != nil && size > 0) {
			size_t n = m_size;
			resize(m_size + size);
			copy(m_buffer + n, (const buffer_t *const)data, size);
			terminate();
		}
	}
	void assign(const TDataBuffer<buffer_t>& buffer) {
		clear();
		append(buffer.data(), buffer.size());
	}
	void assign(const void *const data, std::size_t size) {
		clear();
		append(data, size);
	}
	void swap(TDataBuffer<buffer_t>& buffer) {
		std::size_t sm_size = buffer.m_size;
		std::size_t sm_ordinal = buffer.m_ordinal;
		std::size_t sm_capacity = buffer.m_capacity;
		buffer_t* sm_buffer = buffer.m_buffer;
		buffer_t* sd_buffer = buffer.d_buffer;
		bool sm_owner = buffer.m_owner;

		m_size = buffer.m_size;
		m_ordinal = buffer.m_ordinal;
		m_capacity = buffer.m_capacity;
		m_buffer = buffer.m_buffer;
		d_buffer = buffer.d_buffer;
		m_owner = buffer.m_owner;

		buffer.m_size = sm_size;
		buffer.m_ordinal = sm_ordinal;
		buffer.m_capacity = sm_capacity;
		buffer.m_buffer = sm_buffer;
		buffer.d_buffer = sd_buffer;
		buffer.m_owner = sm_owner;
	}
	void move(TDataBuffer<buffer_t>& buffer) {
		clear();
		if (!buffer.empty()) {
			m_size = buffer.m_size;
			m_ordinal = buffer.m_ordinal;
			m_capacity = buffer.m_capacity;
			m_buffer = buffer.m_buffer;
			d_buffer = buffer.d_buffer;
			m_owner = buffer.m_owner;

			buffer.m_buffer = nil;
			buffer.m_size = 0;
			buffer.clear();
		}
	}
	void move(buffer_t *const data, std::size_t size) {
		clear();
		if (util::assigned(data) && size > 0) {
			m_capacity = m_size = size;
			m_buffer = data;
		}
	}
	void push_back(const buffer_t value) {
		size_t n = m_size;
		resize(m_size + 1);
		m_buffer[n] = value;
	}
	void add(const buffer_t value) {
		push_back(value);
	}
	void fillchar(char n) {
		::memset(m_buffer, n, offset(m_size));
	}
	void fill(buffer_t n) {
		for (size_t i=0; i<m_size; i++) {
			m_buffer[i] = n;
		}
	}

	size_t strlen() const {
		if (!empty())
			return strnlen(data(), offset(m_size));
		return (size_t)0;
	}

	void clear() {
		destroy();
		reset();
	}
	void release() {
		rewind();
	}

	// Operators
	buffer_t* operator () () const {
		return data();
	}
	buffer_t& operator [] (const std::size_t index) const {
		if (validIndex(index)) {
			return m_buffer[index];
		} else {
			// Content may have been overwritten by caller
			// --> set to null again!
			if (nil == d_buffer)
				d_buffer = new buffer_t[1];
			d_buffer[0] = (buffer_t)0;
			return d_buffer[0];
		}
	}

	TDataBuffer<buffer_t>& operator = (const TDataBuffer<buffer_t>& value) {
		assign(value.data(), value.size());
		return *this;
	}
	TDataBuffer<buffer_t>& operator = (const TDataBuffer<buffer_t>&& value) {
		move(value);
		return *this;
	}

	// Default constructor
	TDataBuffer() {
		init(0);
	}

	// Copy constructor
	TDataBuffer(const TDataBuffer<buffer_t>& o) {
		init(0);
		assign(o);
	}
	// Move constructor
	TDataBuffer(TDataBuffer<buffer_t>&& o) {
		init(0);
		move(o);
	};

	// Specialized constructors
	TDataBuffer(const buffer_t* data, const size_t size) {
		init(0);
		assign(data, size);
	};
	TDataBuffer(std::size_t size, bool owner) {
		init(size);
		m_owner = owner;
	}
	TDataBuffer(std::size_t size) {
		init(size);
	}

    virtual ~TDataBuffer() {
    	if (m_owner) {
    		clear();
    	} else {
			// Clear only internal properties, but retain allocated memory
    		reset();
    	}
    }
};


#ifdef STL_HAS_TEMPLATE_ALIAS

using TBuffer = TDataBuffer<char>;
using TCharPointerArray = TDataBuffer<char const *>;
using TBufferA = TBuffer;
using TBufferW = TDataBuffer<wchar_t>;
using TStringBuffer = TBufferA;
using TWideBuffer = TBufferW;
using TByteBuffer = TDataBuffer<uint8_t>;

#else

typedef TDataBuffer<char> TBuffer;
typedef TDataBuffer<char const *> TCharPointerArray;
typedef TBuffer TBufferA;
typedef TDataBuffer<wchar_t> TBufferW;
typedef TBufferA TStringBuffer;
typedef TBufferW TWideBuffer;
typedef TDataBuffer<uint8_t> TByteBuffer;

#endif


class TMemoryLock {
protected:
	bool lockAddress(void * addr, const size_t size) {
		// Lock memory region
		errno = EXIT_SUCCESS;
		int r = mlock(addr, size);
		if (EXIT_SUCCESS == r) {
			// Access memory region and write 0
			// --> Prevent delayed copy-on-write page faults in critical application sections
			memset(addr, 0, size);
			return true;
		}
		return false;
	}

	bool unlockAddress(void * addr, const size_t size) {
		// Lock memory region
		errno = EXIT_SUCCESS;
		return EXIT_SUCCESS == munlock(addr, size);
	}
};


template<typename T>
class TMappedMemory : public app::TObject, private TMemoryLock {
private:
	typedef T buffer_t;

	buffer_t* mmem;
	size_t size;
	size_t aligned;
	bool locked;
	int errval;
	std::string errmsg;

	void clear() {
		mmem = nil;
		errval = EXIT_SUCCESS;
		size = 0;
		locked = false;
	}

public:
	int error() const { return errval; }
	const std::string& message() const { return errmsg; }
	bool empty() const { return !util::assigned(mmem); }
	buffer_t* data() const { return mmem; }

	int create(buffer_t** addr, const size_t size) {
		int r;
		*addr = nil;

		if (util::assigned(mmem)) {
			if (this->size == size) {
				*addr = mmem;
				return EXIT_SUCCESS;
			}
			r = destroy();
			if (EXIT_SUCCESS != r)
				return r;
		}

		// Create aligned memory region
		ssize_t alignment = sysconf(_SC_PAGE_SIZE);
		if (alignment <= 0) {
			errmsg = "TMappedMemory::create() failed: Invalid alignment size.";
			errval = EINVAL;
			return EXIT_FAILURE;
		}
		size_t bytes = size * sizeof(buffer_t);
		size_t carry = bytes % (size_t)alignment;
		aligned = carry > 0 ? bytes + (size_t)alignment - carry : bytes;

		// Map memory region
		void* p = nil;
		p = mmap(NULL, aligned, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (MAP_FAILED == p) {
			errmsg = "TMappedMemory::create()::mmap() failed.";
			errval = errno;
			return EXIT_FAILURE;
		}

		// Return pointer to memory map
		if (util::assigned(p)) {
			this->size = size;
			*addr = mmem = (buffer_t*)p;
			return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	int destroy() {
		int r;
		int retVal = EXIT_SUCCESS;
		errval = EXIT_SUCCESS;

		if (size <= 0 || !util::assigned(mmem)) {
			size = 0;
			return errval;
		}

		// Unmap shared memory
		r = munmap(mmem, aligned);
		if (EXIT_SUCCESS != r) {
			errmsg = "TMappedMemory::destroy()::munmap() for mapped memory failed.";
			errval = errno;
			retVal = EXIT_FAILURE;
		}

		clear();
		aligned = 0;
		return retVal;
	}

	bool lock() {
		errno = errval = EXIT_SUCCESS;
		if (locked)
			return true;
		if (util::assigned(mmem) && aligned > 0) {
			if (lockAddress(mmem, aligned)) {
				locked = true;
				return true;
			} else {
				errmsg = "TMappedMemory::lock()::lockAddress() for mapped memory failed.";
			}
		} else {
			errmsg = "TMappedMemory::lock()::unlockAddress() failed on empty mapped memory buffer";
			errno = EINVAL;
		}
		errval = errno;
		return false;
	}

	bool unlock() {
		errno = errval = EXIT_SUCCESS;
		if (!locked)
			return true;
		if (util::assigned(mmem) && aligned > 0) {
			if (unlockAddress(mmem, aligned)) {
				locked = false;
				return true;
			} else {
				errmsg = "TMappedMemory::unlock()::unlockAddress() for mapped memory failed.";
			}
		} else {
			errmsg = "TMappedMemory::unlock()::unlockAddress() failed on empty mapped memory buffer";
			errno = EINVAL;
		}
		errval = errno;
		return false;
	}

	explicit TMappedMemory() { clear(); }
	virtual ~TMappedMemory() { destroy(); }
};


template<typename T>
class TSharedMemory : public app::TObject, private TMemoryLock {
private:
	typedef T buffer_t;

	buffer_t* shm;
	int handle;
	size_t size;
	size_t aligned;
	bool locked;
	int errval;
	std::string errmsg;

	void clear() {
		shm = nil;
		errval = EXIT_SUCCESS;
		handle = INVALID_HANDLE_VALUE;
		locked = false;
		size = 0;
	}

public:
	int error() const { return errval; }
	const std::string& message() const { return errmsg; }
	bool empty() const { return !util::assigned(shm); }
	buffer_t* data() const { return shm; }

	std::string validateName(const std::string& name) {
		std::string s = name;
		if (s == "*")
			s = util::fastCreateUUID();
		if (s[0] != '/')
			s.insert(0, 1, '/');
		return s;
	}

	int create(buffer_t** addr, const size_t size, const std::string region = "*", const mode_t mode = SHM_DEFAULT_ACL) {
		int r;
		*addr = nil;
		name = validateName(region);

		if (util::assigned(shm)) {
			if (this->size == size) {
				*addr = shm;
				return EXIT_SUCCESS;
			}
			r = destroy();
			if (EXIT_SUCCESS != r)
				return r;
		}

		// Create aligned memory region
		ssize_t alignment = sysconf(_SC_PAGE_SIZE);
		if (alignment <= 0) {
			errmsg = "TSharedMemory::create() failed: Invalid alignment size.";
			errval = EINVAL;
			return EXIT_FAILURE;
		}
		size_t bytes = size * sizeof(buffer_t);
		size_t carry = bytes % (size_t)alignment;
		aligned = carry > 0 ? bytes + (size_t)alignment - carry : bytes;

		// Get handle to shared memory region
		if (handle < 0) {
			handle = shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, mode);
			if (handle < 0) {
				errmsg = "TSharedMemory::create()::shm_open() failed.";
				errval = errno;
				return EXIT_FAILURE;
			}
		}

		// Set expected size for memory mapped region
		r = ftruncate(handle, aligned);
		if (EXIT_SUCCESS != r) {
			errmsg = "TSharedMemory::create()::ftruncate() for shared memory handle failed.";
			errval = errno;
			return EXIT_FAILURE;
		}

		// Map memory region
		void* p = nil;
		p = mmap(nil, aligned, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
		if (MAP_FAILED == p) {
			errmsg = "TSharedMemory::create()::mmap() failed.";
			errval = errno;
			return EXIT_FAILURE;
		}

		// Return pointer to memory map
		if (util::assigned(p)) {
			this->size = size;
			*addr = shm = (buffer_t*)p;
			return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	int destroy() {
		int r;
		int retVal = EXIT_SUCCESS;
		errval = EXIT_SUCCESS;

		if (size <= 0 || !util::assigned(shm)) {
			size = 0;
			return errval;
		}

		// Unmap shared memory
		r = munmap(shm, aligned);
		if (EXIT_SUCCESS != r) {
			errmsg = "TSharedMemory::destroy()::munmap() for shared memory failed.";
			errval = errno;
			retVal = EXIT_FAILURE;
		}

		// Release named memory region
		if (handle >= 0) {
			r = shm_unlink(name.c_str());
			if (EXIT_SUCCESS == r) {
				do {
					errno = EXIT_SUCCESS;
					r = ::close(handle);
				} while (r == EXIT_ERROR && errno == EINTR);
				if (EXIT_SUCCESS != r) {
					errmsg = "TSharedMemory::destroy()::close() for shared memory failed.";
					errval = errno;
					retVal = EXIT_FAILURE;
				}
			} else {
				errmsg = "TSharedMemory::destroy()::shm_unlink() for shared memory failed.";
				errval = errno;
				retVal = EXIT_FAILURE;
			}
		}

		clear();
		aligned = 0;
		return retVal;
	}

	bool lock() {
		errno = errval = EXIT_SUCCESS;
		if (locked)
			return true;
		if (util::assigned(shm) && aligned > 0) {
			if (lockAddress(shm, aligned)) {
				locked = true;
				return true;
			} else {
				errmsg = "TSharedMemory::lock()::lockAddress() for shared memory failed.";
			}
		} else {
			errmsg = "TSharedMemory::lock()::lockAddress() failed on empty shared memory buffer";
			errno = EINVAL;
		}
		errval = errno;
		return false;
	}

	bool unlock() {
		errno = errval = EXIT_SUCCESS;
		if (!locked)
			return true;
		if (util::assigned(shm) && aligned > 0) {
			if (unlockAddress(shm, aligned)) {
				locked = false;
				return true;
			} else {
				errmsg = "TSharedMemory::unlock()::unlockAddress() for shared memory failed.";
			}
		} else {
			errmsg = "TSharedMemory::unlock()::lockAddress() failed on empty shared memory buffer";
			errno = EINVAL;
		}
		errval = errno;
		return false;
	}

	explicit TSharedMemory() { clear(); }
	virtual ~TSharedMemory() { destroy(); }
};


template<typename T>
class TMappedBuffer : public TDataBuffer<T> {
private:
	typedef TDataBuffer<T> TBuffer;
	typedef typename TBuffer::buffer_t buffer_t;

	TMappedMemory<buffer_t> mmem;
	buffer_t* mptr;

	buffer_t* create(size_t size) {
		if (util::assigned(mptr))
			destroy();
		int r = mmem.create(&mptr, size);
		if (r != EXIT_SUCCESS || !util::assigned(mptr))
			throw util::sys_error("TMappedBuffer::create() : " + mmem.message(), mmem.error());
		return mptr;
	}

	void destroy() {
		mmem.destroy();
		mptr = nil;
	}


public:
	int error() const { return mmem.error(); }
	const std::string& message() const { return mmem.message(); }
	buffer_t* pointer() const { return mptr; }

	bool lock() { return mmem.lock(); }
	bool unlock() { return mmem.unlock(); }

	TMappedBuffer() : TBuffer(0) {
		mptr = nil;
		this->init(0, 0);
	}

	TMappedBuffer(std::size_t size) : TBuffer(0) {
		mptr = nil;
		this->init(size, size);
	}

    ~TMappedBuffer() {
    	this->clear();
    }
};


template<typename T>
class TSharedBuffer : public TDataBuffer<T> {
private:
	typedef TDataBuffer<T> TBuffer;
	typedef typename TBuffer::buffer_t buffer_t;

	mode_t mode;
	std::string name;
	TSharedMemory<buffer_t> shm;
	buffer_t* sptr;

	buffer_t* create(size_t size) {
		if (util::assigned(sptr))
			destroy();
		int r = shm.create(&sptr, size, name, mode);
		if (r != EXIT_SUCCESS || !util::assigned(sptr))
			throw util::sys_error("TSharedBuffer::create() : " + shm.message(), shm.error());
		return sptr;
	}

	void destroy() {
		shm.destroy();
		sptr = nil;
	}


public:
	int error() const { return shm.error(); }
	const std::string& message() const { return shm.message(); }
	buffer_t* pointer() const { return sptr; }

	bool lock() { return shm.lock(); }
	bool unlock() { return shm.unlock(); }

	TSharedBuffer() : TBuffer(0), mode(SHM_DEFAULT_ACL) {
		sptr = nil;
		this->name = shm.validateName("*");
		this->init(0, 0);
	}

	TSharedBuffer(std::size_t size, mode_t mode = SHM_DEFAULT_ACL) : TBuffer(0), mode(mode) {
		sptr = nil;
		this->name = shm.validateName("*");
		this->init(size, size);
	}

	TSharedBuffer(std::size_t size, const std::string& name, mode_t mode = SHM_DEFAULT_ACL) : TBuffer(0), mode(mode) {
		sptr = nil;
		this->name = shm.validateName(name);
		this->init(size, size);
	}

    ~TSharedBuffer() {
    	this->clear();
    }
};

} /* namespace util */

#endif /* MEMORY_H_ */
