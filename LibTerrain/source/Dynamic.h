#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>
#include <algorithm>
#include <typeinfo>

#include "../../LibGL/source/utils.h"

#define DYNAMIC_POOL_STRICT

/**
 * File: dynamic.hpp
 * -----------------------
 * A generic dynamic object pool class for memory reuse and fast allocation.
 * This template class manages allocation and deallocation of objects of type T
 * by keeping a free list of unused objects and avoiding frequent heap allocations.
 */
template <typename T>
class CDynamicPool
{
public:

	/**
	 * CDynamicPool - Constructor
	 * Initializes the object pool with zero capacity and usage.
	 */
	CDynamicPool()
	{
		m_uiInitCapacity = 0;
		m_uiUsedCapacity = 0;
	}

	/**
	 * ~CDynamicPool - Destructor
	 * Ensures all allocated objects have been freed before cleanup.
	 * Frees all memory allocated by the pool.
	 */
	virtual ~CDynamicPool()
	{
		assert(m_vpData.empty());
#if defined(ENABLE_DYNAMIC_POOL_LOGS)
		char szText[256];
		snprintf(szText, sizeof(szText), "[CDynamicPool<%s>] Final used capacity: %d", typeid(T).name(), m_uiUsedCapacity);
		sys_log("%s", szText);
#endif
	}

	/**
	 * SetName - Setting the name of the pool.
	 * @stName: Optional name for identifying the pool.
	 */
	void SetName(const std::string& stName)
	{
		m_stPoolName = stName;
	}

	/**
	 * Clear - Destroys all pooled objects and resets internal containers.
	 */
	void Clear()
	{
		Destroy();
	}

	/**
	 * Destroy - Deletes all allocated objects and clears the pool.
	 * Frees memory for all objects in both used and free lists.
	 */
	void Destroy()
	{
#if defined(ENABLE_DYNAMIC_POOL_LOGS)
		if (!m_vpData.empty())
		{
			char szText[256];
			snprintf(szText, sizeof(szText), "[CDynamicPool<%s>] Destroying %zu objects", typeid(T).name(), m_vpData.size());
			sys_log("%s", szText);
		}
#endif
		std::for_each(m_vpData.begin(), m_vpData.end(), Delete);
		m_vpData.clear();
		m_vpFree.clear();
	}

	/**
	 * Create - Initializes the pool with a reserved capacity.
	 * @uiCapacity: Estimated initial number of objects to allocate space for.
	 */
	void Create(GLuint uiCapacity)
	{
		m_uiInitCapacity = uiCapacity;
		m_vpData.reserve(uiCapacity);
		m_vpFree.reserve(uiCapacity);
	}

	/**
	 * Alloc - Allocates a new object from the pool.
	 * If a free object is available, reuses it; otherwise creates a new one.
	 * Return: Pointer to the allocated object.
	 */
	T* Alloc()
	{
		if (m_vpFree.empty())
		{
			T* pNewData = new T;
			m_vpData.push_back(pNewData);
			m_uiUsedCapacity++;

#if defined(ENABLE_DYNAMIC_POOL_LOGS)
			sys_log("[CDynamicPool<%s>] Allocated new object (total: %d)", typeid(T).name(), m_uiUsedCapacity);
#endif

			return pNewData;
		}

		T* pFreeData = m_vpFree.back();
		m_vpFree.pop_back();

#if defined(ENABLE_DYNAMIC_POOL_LOGS)
		sys_log("[CDynamicPool<%s>] Reused object from free list", typeid(T).name());
#endif
		return (pFreeData);
	}

	/**
	 * Free - Returns an object to the pool for future reuse.
	 * @pData: Pointer to the object to recycle.
	 */
	void Free(T* pData)
	{
#if defined(DYNAMIC_POOL_STRICT)
		assert(IsValidData(pData));
		assert(!IsFreeData(pData));
#endif
		m_vpFree.push_back(pData);

#if defined(ENABLE_DYNAMIC_POOL_LOGS)
		sys_log("[CDynamicPool<%s>] Freed object", typeid(T).name());
#endif
	}

	/**
	 * FreeAll - Marks all allocated objects as free.
	 * Recycles all objects in use back into the free list.
	 */
	void FreeAll()
	{
		m_vpFree = m_vpData;

#if defined(ENABLE_DYNAMIC_POOL_LOGS)
		sys_log("[CDynamicPool<%s>] All objects marked as free", typeid(T).name());
#endif
	}

	/**
	 * GetCapacity - Returns total number of allocated objects.
	 * Return: Number of objects allocated by the pool.
	 */
	size_t GetCapacity()
	{
		return (m_vpData.size());
	}

	/**
	 * GetUsedCount - Returns total number of used objects.
	 * Return: Number of objects used by the pool.
	 */
	size_t GetUsedCount() const
	{
		return m_vpData.size() - m_vpFree.size();
	}

	/**
	 * GetFreeCount - Returns total number of freed objects.
	 * Return: Number of objects freed - available in the pool.
	 */
	size_t GetFreeCount() const
	{
		return m_vpFree.size();
	}

	/**
	 * GetTotalCount - Returns total number of allocated objects.
	 * Return: Number of objects allocated by the pool.
	 */
	size_t GetTotalCount() const
	{
		return m_vpData.size();
	}

protected:

	/**
	 * IsValidData - Checks if an object belongs to the pool.
	 * @pData: Pointer to the object to check.
	 * Return: true if the object is in the pool; false otherwise.
	 */
	bool IsValidData(T* pData)
	{
		if (m_vpData.end() == std::find(m_vpData.begin(), m_vpData.end(), pData))
		{
			return (false);
		}
		return (true);
	}

	/**
	 * IsFreeData - Checks if an object is currently in the free list.
	 * @pkData: Pointer to the object to check.
	 * Return: true if the object is already freed; false otherwise.
	 */
	bool IsFreeData(T* pData)
	{
		if (m_vpFree.end() == std::find(m_vpFree.begin(), m_vpFree.end(), pData))
		{
			return (false);
		}
		return (true);
	}

	/**
	 * Delete - Static helper to delete a pointer.
	 * @pData: Pointer to delete.
	 */
	static void Delete(T* pData)
	{
		safe_delete(pData);
	}

protected:

	/* All allocated objects */
	std::vector<T*> m_vpData;

	/* Pool of free/reusable objects */
	std::vector<T*> m_vpFree;

	/* Initial reservation size */
	GLuint m_uiInitCapacity;

	/* Number of created (ever) objects */
	GLuint m_uiUsedCapacity;

	/* Dynamic Pool name */
	std::string m_stPoolName;
};


/*
 * custom smart pointer implementation
 */
template <typename T>
class CDynamicPtr
{
public:
	/**
	 * CDynamic - Default constructor.
	 * Initializes internal object pointer to nullptr.
	 */
	CDynamicPtr()
	{
		Initialize();
	}

	/**
	 * ~CDynamic - Destructor.
	 * Frees the object if allocated and returns it to the pool.
	 */
	~CDynamicPtr()
	{
		Clear();
	}

	/**
	 * Clear - Frees the currently allocated object (if any)
	 * and resets the internal pointer.
	 */
	void Clear()
	{
		if (m_pObject)
		{
			ms_objectsPool.Free(m_pObject);
		}

		Initialize();
	}

	/**
	 * GetUsablePointer - Allocates an object from the pool (if not already allocated)
	 * and returns a pointer to it.
	 * Return: pointer to the internal object.
	 */
	T* GetUsablePointer()
	{
		if (!m_pObject)
		{
			m_pObject = ms_objectsPool.Alloc();	
		}

		return (m_pObject);
	}

	/**
	 * IsNull - Checks whether the internal object is null (not allocated).
	 * Return: true if object is null, false otherwise.
	 */
	bool IsNull() const
	{
		if (m_pObject)
		{
			return (false);
		}

		return (true);
	}

	/**
	 * GetPointer - Returns the internal object pointer.
	 * Assert: the object must not be null.
	 * Return: pointer to the object.
	 */
	T* GetPointer() const
	{
		assert(m_pObject);
		return (m_pObject);
	}

	/**
	 * operator-> - Overloads the -> operator to access object members.
	 * Assert: the object must not be null.
	 * Return: pointer to the object.
	 */
	T* operator->() const
	{
		assert(m_pObject);
		return (m_pObject);
	}

private:
	/**
	 * Initialize - Resets the internal object pointer to null.
	 */
	void Initialize()
	{
		m_pObject = nullptr;
	}

private:

	/* Pointer to the pooled object */
	T* m_pObject;

	/* Shared static object pool for all CDynamic<T> instances */
	static CDynamicPool<T> ms_objectsPool;
};

/**
 * Template static member definition.
 * Each CDynamic<T> specialization shares its own pool.
 */
template <typename T>
CDynamicPool<T> CDynamicPtr<T>::ms_objectsPool;
