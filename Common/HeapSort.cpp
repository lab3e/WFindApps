// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Louis A. Bloomfield
// Function: HeapSort
// Purpose: Sorts two tables together to put the first table in numerical order
// Rewritten to use std::sort to eliminate 1-based indexing UB.

#include "stdafx.h"	// the including project's stdafx.h, found via its include path
#include <algorithm>
#include <vector>
#include "HeapSort.h"

void HeapSort(unsigned long *tableA, int *tableB, int n)
{
	std::vector<std::pair<unsigned long, int>> pairs(n);
	for (int i = 0; i < n; i++)
		pairs[i] = { tableA[i], tableB[i] };
	std::sort(pairs.begin(), pairs.end());
	for (int i = 0; i < n; i++)
	{
		tableA[i] = pairs[i].first;
		tableB[i] = pairs[i].second;
	}
}
