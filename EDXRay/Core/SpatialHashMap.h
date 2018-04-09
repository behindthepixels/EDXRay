#pragma once

#include "Core/TypeHash.h"
#include "Math/EDXMath.h"
#include "Containers/Array.h"
#include "Windows/Atomics.h"
#include "Windows/Threading.h"

namespace EDX
{
	template<typename KeyType, typename ValueType>
	class SpatialHashMap
	{
	public:
		struct Entry
		{
			KeyType Key = INDEX_NONE;
			ValueType Value;

			Entry() = default;
			Entry(const KeyType& InKey, const ValueType& InVal)
				: Key(InKey)
				, Value(InVal)
			{
			}
		};

		uint32 mSize;
		AtomicCounter mCounter;
		Array<uint32> mUniqueIds;
		Array<Entry> mEntries;

		SpatialHashMap(const uint32 size)
		{
			Resize(size);
			mCounter.Reset();
		}

		void Resize(const int newSize)
		{
			mSize = Math::RoundUpPowOfTwo(newSize);
			mEntries.Resize(mSize);
			mUniqueIds.Resize(mSize);
		}

		void Insert(const KeyType& key, const ValueType& value)
		{
			const uint32 baseSlot = GetTypeHash(key) & (mSize - 1);
			uint32 slot;
			KeyType oldKey = INDEX_NONE;

			uint32 i = 0;
			do {
				slot = (baseSlot + i * i) & (mSize - 1);
				i++;

				oldKey = WindowsAtomics::InterlockedCompareExchange((int64*)&mEntries[slot].Key, (int64)key, (int64)INDEX_NONE);
			} while (oldKey != INDEX_NONE && oldKey != key);

			if (oldKey == INDEX_NONE)
			{
				// Pack the 
				int counter = mCounter.Add(1);
				mUniqueIds[counter] = slot;
				mEntries[slot].Value = value;
			}

			return;
		}

		ValueType* Find(const KeyType& key)
		{
			const uint32 baseSlot = GetTypeHash(key) & (mSize - 1);
			uint32 slot = baseSlot;

			for (uint32 i = 1; mEntries[slot].Key != INDEX_NONE && mEntries[slot].Key != key; i++)
			{
				slot = (baseSlot + i * i) & (mSize - 1);
			}

			if (mEntries[slot].Key == key)
			{
				return &mEntries[slot].Value;
			}

			return nullptr;
		}

		SIZE_T GetSize() const
		{
			return mSize;
		}
	};
}