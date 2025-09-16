/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <RakNet/DS_Table.h>
#include <RakNet/DS_OrderedList.h>
#include <string.h>
#include <RakNet/RakAssert.h>
#include <RakNet/RakAssert.h>
#include <string>
#include <string_view>
#include <format>

using namespace DataStructures;

#ifdef _MSC_VER
#pragma warning( push )
#endif

void ExtendRows(Table::Row* input, const int index)
{
	(void) index;
	input->cells.Insert(RakNet::OP_NEW<Table::Cell>(_FILE_AND_LINE_), _FILE_AND_LINE_ );
}
void FreeRow(Table::Row* input, const int index)
{
	(void) index;

  for (unsigned i = 0; i < input->cells.Size(); ++i)
	{
		RakNet::OP_DELETE(input->cells[i], _FILE_AND_LINE_);
	}
	RakNet::OP_DELETE(input, _FILE_AND_LINE_);
}
Table::Cell::Cell()
{
	isEmpty=true;
	c=nullptr;
	ptr=nullptr;
	i=0.f;
}
Table::Cell::~Cell()
{
	Clear();
}
Table::Cell& Table::Cell::operator = ( const Table::Cell& input )
{
	isEmpty=input.isEmpty;
	i=input.i;
	ptr=input.ptr;
	if (c)
		rakFree_Ex(c, _FILE_AND_LINE_);
	if (input.c)
	{
		c = static_cast<char*>(rakMalloc_Ex(static_cast<int>(i), _FILE_AND_LINE_));
		memcpy(c, input.c, static_cast<int>(i));
	}
	else
		c=nullptr;
	return *this;
}
Table::Cell::Cell( const Table::Cell & input)
{
	isEmpty=input.isEmpty;
	i=input.i;
	ptr=input.ptr;
	if (input.c)
	{
		if (c)
			rakFree_Ex(c, _FILE_AND_LINE_);
		c =  static_cast<char*>(rakMalloc_Ex(static_cast<int>(i), _FILE_AND_LINE_));
		memcpy(c, input.c, static_cast<int>(i));
	}
}
void Table::Cell::Set(double input)
{
	Clear();
	i=input;
	c=nullptr;
	ptr=nullptr;
	isEmpty=false;
}
void Table::Cell::Set(unsigned int input)
{
	Set(static_cast<int>(input));
}
void Table::Cell::Set(int input)
{
	Clear();
	i= static_cast<double>(input);
	c=nullptr;
	ptr=nullptr;
	isEmpty=false;
}

void Table::Cell::Set(const char *input)
{
	Clear();
		
	if (input)
	{
		i= static_cast<int>(strlen(input)) +1;
		c =  static_cast<char*>(rakMalloc_Ex(static_cast<int>(i), _FILE_AND_LINE_));
		strcpy(c, input);
	}
	else
	{
		c=nullptr;
		i=0;
	}
	ptr=nullptr;
	isEmpty=false;
}
void Table::Cell::Set(const char *input, int inputLength)
{
	Clear();
	if (input)
	{
		c = static_cast<char*>(rakMalloc_Ex(inputLength, _FILE_AND_LINE_));
		i=inputLength;
		memcpy(c, input, inputLength);
	}
	else
	{
		c=nullptr;
		i=0;
	}
	ptr=nullptr;
	isEmpty=false;
}
void Table::Cell::SetPtr(void* p)
{
	Clear();
	c=nullptr;
	ptr=p;
	isEmpty=false;
}
void Table::Cell::Get(int *output)
{
	RakAssert(isEmpty==false);
  const int o = static_cast<int>(i);
	*output=o;
}
void Table::Cell::Get(double *output)
{
	RakAssert(isEmpty==false);
	*output=i;
}
void Table::Cell::Get(char *output)
{
	RakAssert(isEmpty==false);
	strcpy(output, c);
}
void Table::Cell::Get(char *output, int *outputLength)
{
	RakAssert(isEmpty==false);
	memcpy(output, c, static_cast<int>(i));
	if (outputLength)
		*outputLength= static_cast<int>(i);
}
RakNet::RakString Table::Cell::ToString(const ColumnType columnType)
{
	if (isEmpty)
		return RakNet::RakString();

	if (columnType==NUMERIC)
	{
		return RakNet::RakString("%f", i);
	}
	else if (columnType==STRING)
	{
		return RakNet::RakString(c);
	}
	else if (columnType==BINARY)
	{
		return RakNet::RakString("<Binary>");
	}
	else if (columnType==POINTER)
	{
		return RakNet::RakString("%p", ptr);
	}

	return RakNet::RakString();
}
Table::Cell::Cell(const double numericValue, char *charValue, void *ptr,
                  const ColumnType type)
{
	SetByType(numericValue,charValue,ptr,type);
}
void Table::Cell::SetByType(const double numericValue, char *charValue, void *ptr, const ColumnType type)
{
	isEmpty=true;
	if (type==NUMERIC)
	{
		Set(numericValue);
	}
	else if (type==STRING)
	{
		Set(charValue);
	}
	else if (type==BINARY)
	{
		Set(charValue, static_cast<int>(numericValue));
	}
	else if (type==POINTER)
	{
		SetPtr(ptr);
	}
	else
	{
		ptr= static_cast<void*>(charValue);
	}
}
Table::ColumnType Table::Cell::EstimateColumnType(void) const
{
	if (c)
	{
		if (i!=0.0f)
			return BINARY;
		else
			return STRING;
	}

	if (ptr)
		return POINTER;
	return NUMERIC;
}
void Table::Cell::Clear(void)
{
	if (isEmpty==false && c)
	{
		rakFree_Ex(c, _FILE_AND_LINE_);
		c=nullptr;
	}
	isEmpty=true;
}
Table::ColumnDescriptor::ColumnDescriptor()
{

}
Table::ColumnDescriptor::~ColumnDescriptor()
{

}
Table::ColumnDescriptor::ColumnDescriptor(const char cn[_TABLE_MAX_COLUMN_NAME_LENGTH], const ColumnType ct)
{
	columnType=ct;
	strcpy(columnName, cn);
}
void Table::Row::UpdateCell(const unsigned columnIndex, const double value)
{
	cells[columnIndex]->Clear();
	cells[columnIndex]->Set(value);

//	cells[columnIndex]->i=value;
//	cells[columnIndex]->c=0;
//	cells[columnIndex]->isEmpty=false;
}
void Table::Row::UpdateCell(const unsigned columnIndex, const char *str)
{
	cells[columnIndex]->Clear();
	cells[columnIndex]->Set(str);
}
void Table::Row::UpdateCell(const unsigned columnIndex, const int byteLength, const char *data)
{
	cells[columnIndex]->Clear();
	cells[columnIndex]->Set(data,byteLength);
}
Table::Table()
{
}
Table::~Table()
{
	Clear();
}
unsigned Table::AddColumn(const char columnName[_TABLE_MAX_COLUMN_NAME_LENGTH],
                          const ColumnType columnType)
{
	if (columnName[0]==0)
		return static_cast<unsigned>(-1);

	// Add this column.
	columns.Insert(Table::ColumnDescriptor(columnName, columnType), _FILE_AND_LINE_);

	// Extend the rows by one
	rows.ForEachData(ExtendRows);

	return columns.Size()-1;
}
void Table::RemoveColumn(const unsigned columnIndex)
{
	if (columnIndex >= columns.Size())
		return;

	columns.RemoveAtIndex(columnIndex);

	// Remove this index from each row.
  DataStructures::Page<unsigned, Row*, _TABLE_BPLUS_TREE_ORDER> *cur = rows.GetListHead();
	while (cur)
	{
		for (int i = 0; i < cur->size; ++i)
		{
			RakNet::OP_DELETE(cur->data[i]->cells[columnIndex], _FILE_AND_LINE_);
			cur->data[i]->cells.RemoveAtIndex(columnIndex);
		}

		cur=cur->next;
	}
}
unsigned Table::ColumnIndex(const char *columnName) const
{
  for (unsigned columnIndex = 0; columnIndex<columns.Size(); ++columnIndex)
		if (strcmp(columnName, columns[columnIndex].columnName)==0)
			return columnIndex;
	return static_cast<unsigned>(-1);
}
unsigned Table::ColumnIndex(char columnName[_TABLE_MAX_COLUMN_NAME_LENGTH]) const
{
	return ColumnIndex(static_cast<const char*>(columnName));
}
char* Table::ColumnName(const unsigned index) const
{
	if (index >= columns.Size())
		return nullptr;
	else
		return (char*)columns[index].columnName;
}
Table::ColumnType Table::GetColumnType(const unsigned index) const
{
	if (index >= columns.Size())
		return static_cast<Table::ColumnType>(0);
	else
		return columns[index].columnType;
}
unsigned Table::GetColumnCount(void) const
{
	return columns.Size();
}
unsigned Table::GetRowCount(void) const
{
	return rows.Size();
}
Table::Row* Table::AddRow(const unsigned rowId)
{
  Row* newRow = RakNet::OP_NEW<Row>(_FILE_AND_LINE_);
	if (rows.Insert(rowId, newRow)==false)
	{
		RakNet::OP_DELETE(newRow, _FILE_AND_LINE_);
		return 0; // Already exists
	}
  for (unsigned rowIndex = 0; rowIndex < columns.Size(); ++rowIndex)
		newRow->cells.Insert( RakNet::OP_NEW<Table::Cell>(_FILE_AND_LINE_), _FILE_AND_LINE_ );
	return newRow;
}
Table::Row* Table::AddRow(unsigned rowId,
                          const DataStructures::List<Cell> &initialCellValues)
{
	Row *newRow = RakNet::OP_NEW<Row>( _FILE_AND_LINE_ );
  for (unsigned rowIndex = 0; rowIndex < columns.Size(); ++rowIndex)
	{
		if (rowIndex < initialCellValues.Size() && initialCellValues[rowIndex].isEmpty==false)
		{
			Table::Cell *c;
			c = RakNet::OP_NEW<Table::Cell>(_FILE_AND_LINE_);
			c->SetByType(initialCellValues[rowIndex].i,initialCellValues[rowIndex].c,initialCellValues[rowIndex].ptr,columns[rowIndex].columnType);
			newRow->cells.Insert(c, _FILE_AND_LINE_ );
		}
		else
			newRow->cells.Insert(RakNet::OP_NEW<Table::Cell>(_FILE_AND_LINE_), _FILE_AND_LINE_ );
	}
	rows.Insert(rowId, newRow);
	return newRow;
}
Table::Row* Table::AddRow(const unsigned rowId,
                          const DataStructures::List<Cell*> &initialCellValues,
                          const bool copyCells)
{
	Row *newRow = RakNet::OP_NEW<Row>( _FILE_AND_LINE_ );
  for (unsigned rowIndex = 0; rowIndex < columns.Size(); ++rowIndex)
	{
		if (rowIndex < initialCellValues.Size() && initialCellValues[rowIndex] && initialCellValues[rowIndex]->isEmpty==false)
		{
			if (copyCells==false)
				newRow->cells.Insert(RakNet::OP_NEW_4<Table::Cell>( _FILE_AND_LINE_, initialCellValues[rowIndex]->i, initialCellValues[rowIndex]->c, initialCellValues[rowIndex]->ptr, columns[rowIndex].columnType), _FILE_AND_LINE_);
			else
			{
				Table::Cell *c = RakNet::OP_NEW<Table::Cell>( _FILE_AND_LINE_ );
				newRow->cells.Insert(c, _FILE_AND_LINE_);
				*c=*(initialCellValues[rowIndex]);
			}
		}
		else
			newRow->cells.Insert(RakNet::OP_NEW<Table::Cell>(_FILE_AND_LINE_), _FILE_AND_LINE_);
	}
	rows.Insert(rowId, newRow);
	return newRow;
}
Table::Row* Table::AddRowColumns(
    const unsigned rowId, const Row *row,
    const DataStructures::List<unsigned>& columnIndices)
{
	Row *newRow = RakNet::OP_NEW<Row>( _FILE_AND_LINE_ );
  for (unsigned columnIndex = 0; columnIndex < columnIndices.Size(); ++columnIndex)
	{
		if (row->cells[columnIndices[columnIndex]]->isEmpty==false)
		{
			newRow->cells.Insert(RakNet::OP_NEW_4<Table::Cell>( _FILE_AND_LINE_, 
				row->cells[columnIndices[columnIndex]]->i,
				row->cells[columnIndices[columnIndex]]->c,
				row->cells[columnIndices[columnIndex]]->ptr,
				columns[columnIndex].columnType
				), _FILE_AND_LINE_);
		}
		else
		{
			newRow->cells.Insert(RakNet::OP_NEW<Table::Cell>(_FILE_AND_LINE_), _FILE_AND_LINE_);
		}
	}
	rows.Insert(rowId, newRow);
	return newRow;
}
bool Table::RemoveRow(const unsigned rowId)
{
  if (Row * out; rows.Delete(rowId, out))
	{
		DeleteRow(out);
		return true;
	}
	return false;
}
void Table::RemoveRows(const Table *tableContainingRowIDs)
{
  const DataStructures::Page<unsigned, Row*, _TABLE_BPLUS_TREE_ORDER> *cur = tableContainingRowIDs->GetRows().GetListHead();
	while (cur)
	{
		for (unsigned i = 0; i < static_cast<unsigned>(cur->size); ++i)
		{
			rows.Delete(cur->keys[i]);
		}
		cur=cur->next;
	}
	return;
}
bool Table::UpdateCell(const unsigned rowId, const unsigned columnIndex,
                       const int value)
{
	RakAssert(columns[columnIndex].columnType==NUMERIC);

  if (Row* row = GetRowByID(rowId))
	{
		row->UpdateCell(columnIndex, value);
		return true;
	}
	return false;
}
bool Table::UpdateCell(const unsigned rowId, const unsigned columnIndex,
                       const char *str)
{
	RakAssert(columns[columnIndex].columnType==STRING);

  if (Row* row = GetRowByID(rowId))
	{
		row->UpdateCell(columnIndex, str);
		return true;
	}
	return false;
}
bool Table::UpdateCell(const unsigned rowId, const unsigned columnIndex,
                       const int byteLength, const char *data)
{
	RakAssert(columns[columnIndex].columnType==BINARY);

  if (Row* row = GetRowByID(rowId))
	{
		row->UpdateCell(columnIndex, byteLength, data);
		return true;
	}
	return false;
}
bool Table::UpdateCellByIndex(const unsigned rowIndex,
                              const unsigned columnIndex, const int value)
{
	RakAssert(columns[columnIndex].columnType==NUMERIC);

  if (Row* row = GetRowByIndex(rowIndex, nullptr))
	{
		row->UpdateCell(columnIndex, value);
		return true;
	}
	return false;
}
bool Table::UpdateCellByIndex(const unsigned rowIndex,
                              const unsigned columnIndex, const char *str)
{
	RakAssert(columns[columnIndex].columnType==STRING);

  if (Row* row = GetRowByIndex(rowIndex, nullptr))
	{
		row->UpdateCell(columnIndex, str);
		return true;
	}
	return false;
}
bool Table::UpdateCellByIndex(const unsigned rowIndex,
                              const unsigned columnIndex, const int byteLength,
                              const char *data)
{
	RakAssert(columns[columnIndex].columnType==BINARY);

  if (Row* row = GetRowByIndex(rowIndex, nullptr))
	{
		row->UpdateCell(columnIndex, byteLength, data);
		return true;
	}
	return false;
}
void Table::GetCellValueByIndex(const unsigned rowIndex,
                                const unsigned columnIndex, int *output)
{
	RakAssert(columns[columnIndex].columnType==NUMERIC);

  if (const Row* row = GetRowByIndex(rowIndex, nullptr))
	{
		row->cells[columnIndex]->Get(output);
	}
}
void Table::GetCellValueByIndex(const unsigned rowIndex,
                                const unsigned columnIndex, char *output)
{
	RakAssert(columns[columnIndex].columnType==STRING);

  if (const Row* row = GetRowByIndex(rowIndex, nullptr))
	{
		row->cells[columnIndex]->Get(output);
	}
}
void Table::GetCellValueByIndex(const unsigned rowIndex,
                                const unsigned columnIndex, char *output, int *outputLength)
{
	RakAssert(columns[columnIndex].columnType==BINARY);

  if (const Row* row = GetRowByIndex(rowIndex, nullptr))
	{
		row->cells[columnIndex]->Get(output, outputLength);
	}
}
Table::FilterQuery::FilterQuery()
{
	columnName[0]=0;
}
Table::FilterQuery::~FilterQuery()
{

}
Table::FilterQuery::FilterQuery(const unsigned column, Cell *cell,
                                const FilterQueryType op)
{
	columnIndex=column;
	cellValue=cell;
	operation=op;
}
Table::Row* Table::GetRowByID(const unsigned rowId) const
{
  if (Row * row; rows.Get(rowId, row))
		return row;
	return nullptr;
}

Table::Row* Table::GetRowByIndex(unsigned rowIndex, unsigned *key) const
{
  const DataStructures::Page<unsigned, Row*, _TABLE_BPLUS_TREE_ORDER> *cur = rows.GetListHead();
	while (cur)
	{
		if (rowIndex < static_cast<unsigned>(cur->size))
		{
			if (key)
				*key=cur->keys[rowIndex];
			return cur->data[rowIndex];
		}
		if (rowIndex <= static_cast<unsigned>(cur->size))
			rowIndex-=cur->size;
		else
			return nullptr;
		cur=cur->next;
	}
	return nullptr;
}

void Table::QueryTable(const unsigned *columnIndicesSubset,
                       const unsigned numColumnSubset, FilterQuery *inclusionFilters,
                       const unsigned numInclusionFilters,
                       const unsigned *rowIds, const unsigned numRowIDs, Table *result)
{
	unsigned i;
	DataStructures::List<unsigned> columnIndicesToReturn;

	// Clear the result table.
	result->Clear();

	if (columnIndicesSubset && numColumnSubset>0)
	{
		for (i=0; i < numColumnSubset; ++i)
		{
			if (columnIndicesSubset[i]<columns.Size())
				columnIndicesToReturn.Insert(columnIndicesSubset[i], _FILE_AND_LINE_);
		}
	}
	else
	{
		for (i=0; i < columns.Size(); ++i)
			columnIndicesToReturn.Insert(i, _FILE_AND_LINE_);
	}

	if (columnIndicesToReturn.Size()==0)
		return; // No valid columns specified

	for (i=0; i < columnIndicesToReturn.Size(); ++i)
	{
		result->AddColumn(columns[columnIndicesToReturn[i]].columnName,columns[columnIndicesToReturn[i]].columnType);
	}

	// Get the column indices of the filter queries.
	DataStructures::List<unsigned> inclusionFilterColumnIndices;
	if (inclusionFilters && numInclusionFilters>0)
	{
		for (i=0; i < numInclusionFilters; ++i)
		{
			if (inclusionFilters[i].columnName[0])
				inclusionFilters[i].columnIndex=ColumnIndex(inclusionFilters[i].columnName);
			if (inclusionFilters[i].columnIndex<columns.Size())
				inclusionFilterColumnIndices.Insert(inclusionFilters[i].columnIndex, _FILE_AND_LINE_);
			else
				inclusionFilterColumnIndices.Insert(static_cast<unsigned>(-1), _FILE_AND_LINE_);
		}
	}

	if (rowIds==nullptr || numRowIDs==0)
	{
		// All rows
    const DataStructures::Page<unsigned, Row*, _TABLE_BPLUS_TREE_ORDER> *cur = rows.GetListHead();
		while (cur)
		{
			for (i=0; i < static_cast<unsigned>(cur->size); ++i)
			{
				QueryRow(inclusionFilterColumnIndices, columnIndicesToReturn, cur->keys[i], cur->data[i], inclusionFilters, result);
			}
			cur=cur->next;
		}
	}
	else
	{
		// Specific rows
		Row *row;
		for (i=0; i < numRowIDs; ++i)
		{
			if (rows.Get(rowIds[i], row))
			{
				QueryRow(inclusionFilterColumnIndices, columnIndicesToReturn, rowIds[i], row, inclusionFilters, result);
			}
		}
	}
}

void Table::QueryRow(
    const DataStructures::List<unsigned> &inclusionFilterColumnIndices,
    const DataStructures::List<unsigned> &columnIndicesToReturn,
    const unsigned key, const Table::Row* row,
    const FilterQuery *inclusionFilters, Table *result)
{

  // If no inclusion filters, just add the row
	if (inclusionFilterColumnIndices.Size()==0)
	{
		result->AddRowColumns(key, row, columnIndicesToReturn);
	}
	else
	{
    bool pass = false;
    // Go through all inclusion filters.  Only add this row if all filters pass.
		for (unsigned j = 0; j<inclusionFilterColumnIndices.Size(); ++j)
		{
      if (const unsigned columnIndex = inclusionFilterColumnIndices[j];
          columnIndex != static_cast<unsigned>(-1) && row->cells[columnIndex]->isEmpty==false )
			{
				if (columns[inclusionFilterColumnIndices[j]].columnType==STRING &&
					(row->cells[columnIndex]->c==nullptr ||
					inclusionFilters[j].cellValue->c==nullptr)	)
					continue;

				switch (inclusionFilters[j].operation)
				{
				case QF_EQUAL:
					switch(columns[inclusionFilterColumnIndices[j]].columnType)
					{
					case NUMERIC:
						pass=row->cells[columnIndex]->i==inclusionFilters[j].cellValue->i;
						break;
					case STRING:
						pass=strcmp(row->cells[columnIndex]->c,inclusionFilters[j].cellValue->c)==0;
						break;
					case BINARY:
						pass=row->cells[columnIndex]->i==inclusionFilters[j].cellValue->i &&
							memcmp(row->cells[columnIndex]->c,inclusionFilters[j].cellValue->c, static_cast<int>(row->cells[columnIndex]->i))==0;
						break;
					case POINTER:
						pass=row->cells[columnIndex]->ptr==inclusionFilters[j].cellValue->ptr;
						break;
					}
					break;
				case QF_NOT_EQUAL:
					switch(columns[inclusionFilterColumnIndices[j]].columnType)
					{
					case NUMERIC:
						pass=row->cells[columnIndex]->i!=inclusionFilters[j].cellValue->i;
						break;
					case STRING:
						pass=strcmp(row->cells[columnIndex]->c,inclusionFilters[j].cellValue->c)!=0;
						break;
					case BINARY:
						pass=row->cells[columnIndex]->i==inclusionFilters[j].cellValue->i &&
							memcmp(row->cells[columnIndex]->c,inclusionFilters[j].cellValue->c, static_cast<int>(row->cells[columnIndex]->i))==0;
						break;
					case POINTER:
						pass=row->cells[columnIndex]->ptr!=inclusionFilters[j].cellValue->ptr;
						break;
					}
					break;
				case QF_GREATER_THAN:
					switch(columns[inclusionFilterColumnIndices[j]].columnType)
					{
					case NUMERIC:
						pass=row->cells[columnIndex]->i>inclusionFilters[j].cellValue->i;
						break;
					case STRING:
						pass=strcmp(row->cells[columnIndex]->c,inclusionFilters[j].cellValue->c)>0;
						break;
					case BINARY:
      					break;
					case POINTER:
						pass=row->cells[columnIndex]->ptr>inclusionFilters[j].cellValue->ptr;
						break;
					}
					break;
				case QF_GREATER_THAN_EQ:
					switch(columns[inclusionFilterColumnIndices[j]].columnType)
					{
					case NUMERIC:
						pass=row->cells[columnIndex]->i>=inclusionFilters[j].cellValue->i;
						break;
					case STRING:
						pass=strcmp(row->cells[columnIndex]->c,inclusionFilters[j].cellValue->c)>=0;
						break;
					case BINARY:
						break;
					case POINTER:
						pass=row->cells[columnIndex]->ptr>=inclusionFilters[j].cellValue->ptr;
						break;
					}
					break;
				case QF_LESS_THAN:
					switch(columns[inclusionFilterColumnIndices[j]].columnType)
					{
					case NUMERIC:
						pass=row->cells[columnIndex]->i<inclusionFilters[j].cellValue->i;
						break;
					case STRING:
						pass=strcmp(row->cells[columnIndex]->c,inclusionFilters[j].cellValue->c)<0;
						break;
					case BINARY:
					  break;
					case POINTER:
						pass=row->cells[columnIndex]->ptr<inclusionFilters[j].cellValue->ptr;
						break;
					}
					break;
				case QF_LESS_THAN_EQ:
					switch(columns[inclusionFilterColumnIndices[j]].columnType)
					{
					case NUMERIC:
						pass=row->cells[columnIndex]->i<=inclusionFilters[j].cellValue->i;
						break;
					case STRING:
						pass=strcmp(row->cells[columnIndex]->c,inclusionFilters[j].cellValue->c)<=0;
						break;
					case BINARY:
						break;
					case POINTER:
						pass=row->cells[columnIndex]->ptr<=inclusionFilters[j].cellValue->ptr;
						break;
					}
					break;
				case QF_IS_EMPTY:
					pass=false;
					break;
				case QF_NOT_EMPTY:
					pass=true;
					break;
				default:
					pass=false;
					RakAssert(0);
					break;
				}
			}
			else
			{
				if (inclusionFilters[j].operation==QF_IS_EMPTY)
					pass=true;
				else
					pass=false; // No value for this cell
			}

			if (pass==false)
				break;
		}

		if (pass)
		{
			result->AddRowColumns(key, row, columnIndicesToReturn);
		}
	}
}

static Table::SortQuery *_sortQueries;
static unsigned _numSortQueries;
static DataStructures::List<unsigned> *_columnIndices;
static DataStructures::List<Table::ColumnDescriptor> *_columns;
int RowSort(Table::Row* const &first, Table::Row* const &second) // first is the one inserting, second is the one already there.
{
  for (unsigned i = 0; i<_numSortQueries; ++i)
	{
    const unsigned columnIndex = (*_columnIndices)[i];
		if (columnIndex== static_cast<unsigned>(-1))
			continue;

		if (first->cells[columnIndex]->isEmpty==true && second->cells[columnIndex]->isEmpty==false)
			return 1; // Empty cells always go at the end

		if (first->cells[columnIndex]->isEmpty==false && second->cells[columnIndex]->isEmpty==true)
			return -1; // Empty cells always go at the end

		if (_sortQueries[i].operation==Table::QS_INCREASING_ORDER)
		{
			if ((*_columns)[columnIndex].columnType==Table::NUMERIC)
			{
				if (first->cells[columnIndex]->i>second->cells[columnIndex]->i)
					return 1;
				if (first->cells[columnIndex]->i<second->cells[columnIndex]->i)
					return -1;
			}
			else
			{
				// String
				if (strcmp(first->cells[columnIndex]->c,second->cells[columnIndex]->c)>0)
					return 1;
				if (strcmp(first->cells[columnIndex]->c,second->cells[columnIndex]->c)<0)
					return -1;
			}
		}
		else
		{
			if ((*_columns)[columnIndex].columnType==Table::NUMERIC)
			{
				if (first->cells[columnIndex]->i<second->cells[columnIndex]->i)
					return 1;
				if (first->cells[columnIndex]->i>second->cells[columnIndex]->i)
					return -1;
			}
			else
			{
				// String
				if (strcmp(first->cells[columnIndex]->c,second->cells[columnIndex]->c)<0)
					return 1;
				if (strcmp(first->cells[columnIndex]->c,second->cells[columnIndex]->c)>0)
					return -1;
			}
		}
	}

	return 0;
}
void Table::SortTable(Table::SortQuery *sortQueries,
                      const unsigned numSortQueries, Table::Row** out)
{
	unsigned i;
	unsigned outLength;
	DataStructures::List<unsigned> columnIndices;
	_sortQueries=sortQueries;
	_numSortQueries=numSortQueries;
	_columnIndices=&columnIndices;
	_columns=&columns;
	bool anyValid=false;

	for (i = 0; i < numSortQueries; ++i) {
    if (sortQueries[i].columnIndex < columns.Size() &&
        columns[sortQueries[i].columnIndex].columnType != BINARY) {
      columnIndices.Insert(sortQueries[i].columnIndex, _FILE_AND_LINE_);
      anyValid = true;
    } else
      columnIndices.Insert(static_cast<unsigned>(-1),
                           _FILE_AND_LINE_); // Means don't check this column
  }

  const DataStructures::Page<unsigned, Row*, _TABLE_BPLUS_TREE_ORDER>* cur =
      rows.GetListHead();
	if (anyValid==false)
	{
		outLength=0;
		while (cur)
		{
			for (i=0; i < static_cast<unsigned>(cur->size); ++i)
			{
				out[(outLength)++]=cur->data[i];
			}
			cur=cur->next;
		}
		return;
	}

	// Start adding to ordered list.
	DataStructures::OrderedList<Row*, Row*, RowSort> orderedList;
	while (cur)
	{
		for (i=0; i < static_cast<unsigned>(cur->size); ++i)
		{
			RakAssert(cur->data[i]);
			orderedList.Insert(cur->data[i],cur->data[i], true, _FILE_AND_LINE_);
		}
		cur=cur->next;
	}

	outLength=0;
	for (i=0; i < orderedList.Size(); ++i)
		out[(outLength)++]=orderedList[i];
}

std::string Table::PrintColumnHeaders(const char columnDelineator) const
{
    std::string out;
    for(size_t i = 0; i < columns.Size(); ++i){
        if(i) out.push_back(columnDelineator);
        std::string_view name = columns[i].columnName;
        out.append(name.data(), name.size());
    }
    return out;
}

std::string Table::PrintRow(const char columnDelineator,
                            const bool printDelineatorForBinary,
                            const Table::Row* inputRow) const
{
    if (inputRow->cells.Size() != columns.Size()) {
        return "Cell width does not match column width.\n";
    }

    const unsigned n = columns.Size();
    std::string out;
    out.reserve(n * 8);

    for (size_t i = 0; i < n; ++i) {
        const auto& col  = columns[i];
        const auto* cell = inputRow->cells[i];

        if (col.columnType == NUMERIC) {
            if (!cell->isEmpty) {
                out += std::format("{:.6f}", cell->i);
            }
        } else if (col.columnType == STRING) {
            if (!cell->isEmpty && cell->c) {
                out.append(cell->c);
            }
        } else if (col.columnType == POINTER) {
            if (!cell->isEmpty && cell->ptr) {
                out += std::format("{:p}", cell->ptr);
            }
        } else {
            //
        }

        if (i + 1 != n) {
            const bool isBinary = !(col.columnType == NUMERIC ||
                                    col.columnType == STRING  ||
                                    col.columnType == POINTER);
            if (!isBinary || printDelineatorForBinary) {
                out.push_back(columnDelineator);
            }
        }
    }

    return out;
}

void Table::Clear(void)
{
	rows.ForEachData(FreeRow);
	rows.Clear();
	columns.Clear(true, _FILE_AND_LINE_);
}
const List<Table::ColumnDescriptor>& Table::GetColumns(void) const
{
	return columns;
}
const DataStructures::BPlusTree<unsigned, Table::Row*, _TABLE_BPLUS_TREE_ORDER>& Table::GetRows(void) const
{
	return rows;
}
DataStructures::Page<unsigned, DataStructures::Table::Row*, _TABLE_BPLUS_TREE_ORDER> * Table::GetListHead(void)
{
	return rows.GetListHead();
}
unsigned Table::GetAvailableRowId(void) const
{
	bool setKey=false;
	unsigned key = 0;
  const DataStructures::Page<unsigned, Row*, _TABLE_BPLUS_TREE_ORDER> *cur = rows.GetListHead();
	
	while (cur)
	{
		for (int i = 0; i < cur->size; ++i)
		{
			if (setKey==false)
			{
				key=cur->keys[i]+1;
				setKey=true;
			}
			else
			{
				if (key!=cur->keys[i])
					return key;
				key++;
			}
		}

		cur=cur->next;
	}
	return key;
}
void Table::DeleteRow(Table::Row *row)
{
  for (unsigned rowIndex = 0; rowIndex < row->cells.Size(); ++rowIndex)
	{
		RakNet::OP_DELETE(row->cells[rowIndex], _FILE_AND_LINE_);
	}
	RakNet::OP_DELETE(row, _FILE_AND_LINE_);
}
Table& Table::operator = ( const Table& input )
{
	Clear();

	unsigned int i;
	for (i = 0; i < input.GetColumnCount(); ++i)
    AddColumn(input.ColumnName(i), input.GetColumnType(i));

  const DataStructures::Page<unsigned, Row*, _TABLE_BPLUS_TREE_ORDER> *cur = input.GetRows().GetListHead();
	while (cur)
	{
		for (i=0; i < static_cast<unsigned int>(cur->size); i++)
		{
			AddRow(cur->keys[i], cur->data[i]->cells, false);
		}

		cur=cur->next;
	}

	return *this;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif
