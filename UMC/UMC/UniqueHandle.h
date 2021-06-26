#pragma once
template<typename T>
struct UniqueHandleTraits
{
};

template<typename T, typename traits = UniqueHandleTraits<T> >
class UniqueHandle
{
protected:
	T handle = traits::invalid_value;
	T exthandle;
public:
	UniqueHandle() : exthandle(handle) {};
	UniqueHandle(T& externalHandle) : exthandle(externalHandle) {}
	UniqueHandle(T&& externalHandle) : handle(externalHandle), exthandle(handle) {}
	constexpr operator T& () { return exthandle; }
	constexpr operator T* () { return &exthandle; }
	virtual ~UniqueHandle()
	{
		Close();
	}
	void Close()
	{
		if (exthandle != traits::invalid_value)
			traits::Close(exthandle);
		exthandle = static_cast<T>(traits::invalid_value);
	}
};

