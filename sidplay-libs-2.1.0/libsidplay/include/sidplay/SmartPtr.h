/* Simple smart pointer class. */

#ifndef SMARTPTR_H
#define SMARTPTR_H

typedef unsigned long int ulint_smartpt;

template <class T>
class SmartPtrBase_sidtt
{
 public:
	
	/* --- constructors --- */
	
	SmartPtrBase_sidtt(T* buffer, ulint_smartpt bufferLen, bool bufOwner = false) : dummy(0)
	{
		doFree = bufOwner;
		if ( bufferLen >= 1 )
		{
			pBufCurrent = ( bufBegin = buffer );
			bufEnd = bufBegin + bufferLen;
			bufLen = bufferLen;
			status = true;
		}
		else
		{
			pBufCurrent = ( bufBegin = ( bufEnd = 0 ));
			bufLen = 0;
			status = false;
		}
	}
	
	/* --- destructor --- */
	
	virtual ~SmartPtrBase_sidtt()
	{
		if ( doFree && (bufBegin != 0) )
		{
#ifndef SID_HAVE_BAD_COMPILER
			delete[] bufBegin;
#else
			delete[] (void*)bufBegin;
#endif
		}
	}
	
	/* --- public member functions --- */
	
	virtual T* tellBegin()  { return bufBegin; }
	virtual ulint_smartpt tellLength()  { return bufLen; }
	virtual ulint_smartpt tellPos()  { return (ulint_smartpt)(pBufCurrent-bufBegin); }

	virtual bool checkIndex(ulint_smartpt index)
	{
		return ((pBufCurrent+index)<bufEnd);
	}
	
	virtual bool reset()
	{
		if ( bufLen >= 1 )
		{
			pBufCurrent = bufBegin;
			return (status = true);
		}
		else
		{
			return (status = false);
		}
	}

	virtual bool good()
	{
		return (pBufCurrent<bufEnd);
	}
	
	virtual bool fail()  
	{
		return (pBufCurrent==bufEnd);
	}
	
	virtual void operator ++()
	{
		if ( good() )
		{
			pBufCurrent++;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator ++(int)
	{
		if ( good() )
		{
			pBufCurrent++;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator --()
	{
		if ( !fail() )
		{
			pBufCurrent--;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator --(int)
	{
		if ( !fail() )
		{
			pBufCurrent--;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator +=(ulint_smartpt offset)
	{
		if (checkIndex(offset))
		{
			pBufCurrent += offset;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator -=(ulint_smartpt offset)
	{
		if ((pBufCurrent-offset) >= bufBegin)
		{
			pBufCurrent -= offset;
		}
		else
		{
			status = false;
		}
	}
	
	virtual T operator*()
	{
		if ( good() )
		{
			return *pBufCurrent;
		}
		else
		{
			status = false;
			return dummy;
		}
	}

	virtual T& operator [](ulint_smartpt index)
	{
		if (checkIndex(index))
		{
			return pBufCurrent[index];
		}
		else
		{
			status = false;
			return dummy;
		}
	}

	virtual operator bool()  { return status; }
 protected:
	
	T* bufBegin;
	T* bufEnd;
	T* pBufCurrent;
	ulint_smartpt bufLen;
	bool status;
	bool doFree;
	T dummy;
	
};


template <class T>
class SmartPtr_sidtt : public SmartPtrBase_sidtt<T>
{
 public:
	
	/* --- constructors --- */
	
	SmartPtr_sidtt(T* buffer, ulint_smartpt bufferLen, bool bufOwner = false)
		: SmartPtrBase_sidtt<T>(buffer, bufferLen, bufOwner)
	{
	}
	
	SmartPtr_sidtt()
		: SmartPtrBase_sidtt<T>(0,0)
	{
	}

	void setBuffer(T* buffer, ulint_smartpt bufferLen)
	{
		if ( bufferLen >= 1 )
		{
			this->pBufCurrent = ( this->bufBegin = buffer );
			this->bufEnd = this->bufBegin + bufferLen;
			this->bufLen = bufferLen;
			this->status = true;
		}
		else
		{
			this->pBufCurrent = this->bufBegin = this->bufEnd = 0;
			this->bufLen = 0;
			this->status = false;
		}
	}
};

#endif  /* SMARTPTR_H */
