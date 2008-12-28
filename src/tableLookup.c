/*	tableLookup.c

	Copyright 2008 Fred Cooke

	This file is part of the FreeEMS project.

	FreeEMS software is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	FreeEMS software is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with any FreeEMS software.  If not, see <http://www.gnu.org/licenses/>.

	We ask that if you make any changes to this file you send them upstream to us at admin@diyefi.org

	Thank you for choosing FreeEMS to run your engine! */

#define TABLELOOKUP_C
#include "inc/freeEMS.h"
#include "inc/commsISRs.h"
#include "inc/tableLookup.h"


/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ******* ******* ******* &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ******* WARNING ******* &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ******* ******* ******* &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&                                                               &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&& These routines rely on the fact that there are no ISRs trying &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&& to access the small tables and other live settings in the ram &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&& window as specified by the RPAGE value. If they are then bad  &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&& values WILL be occasionally read from random parts of the big &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&& tables instead of the correct place. If that occurs it WILL   &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&& cause unpredictable and VERY hard to find bugs!!              &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&                                                               &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&  *******  *******  YOU HAVE BEEN WARNED!!!  *******  *******  &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&                                                               &&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/


/* Yet to be implemented :

unsigned char lookup8Bit3dUC(
unsigned short lookup16Bit2dUS(
unsigned char lookup8Bit2dUC(
signed short lookup16Bit3dSS(
signed short lookup16Bit3dSS(
signed char lookup8Bit3D( */


/* Take a table with two movable axis sets and two axis lengths,
 * loop to find which pairs of axis values and indexs we are between,
 * interpolate two pairs down to two values,
 * interpolate two values down to one value.
 *
 * Warning : This function relies on the axis values being a sorted list from low to high. If this is not the case behaviour is undefined and could include memory corruption and engine damage.
 *
 * To reduce the table size from 19x24 to something smaller, simply reduce the RPMLength and LoadLength fields to lower values. Increasing the size of either axis is not currently possible.
 *
 * Given that the axis lists are in order, a data point outside the table will give the value adjacent to it, and one outside one of the four corners will give the corner value. This is a clean and reasonable behaviour in my opinion.
 *
 * Reminder : X/RPM is horizontal, Y/Load is vertical
 *
 * Save the page value to a variable, change the page value, read the table, change the page back and return the value */
unsigned short lookupPagedMainTableCellValue(mainTable *Table, unsigned short realRPM, unsigned short realLoad, unsigned char RPageValue){

	/* Save the RPAGE value for restoration and switch pages. */
	unsigned char oldRPage = RPAGE;
	RPAGE = RPageValue;

	/* Find the bounding axis values and indices for RPM */
	unsigned char lowRPMIndex = 0;
	unsigned char highRPMIndex = Table->RPMLength - 1;
	/* If never set in the loop, low value will equal high value and will be on the edge of the map */
	unsigned short lowRPMValue = Table->RPM[0];
	unsigned short highRPMValue = Table->RPM[Table->RPMLength -1];

	unsigned char RPMIndex;
	for(RPMIndex=0;RPMIndex<Table->RPMLength;RPMIndex++){
		if(Table->RPM[RPMIndex] < realRPM){
			lowRPMValue = Table->RPM[RPMIndex];
			lowRPMIndex = RPMIndex;
		}else if(Table->RPM[RPMIndex] > realRPM){
			highRPMValue = Table->RPM[RPMIndex];
			highRPMIndex = RPMIndex;
			break;
		}else if(Table->RPM[RPMIndex] == realRPM){
			lowRPMValue = Table->RPM[RPMIndex];
			highRPMValue = Table->RPM[RPMIndex];
			lowRPMIndex = RPMIndex;
			highRPMIndex = RPMIndex;
			break;
		}
	}


	/* Find the bounding cell values and indices for Load */
	unsigned char lowLoadIndex = 0;
	unsigned char highLoadIndex = Table->LoadLength -1;
	/* If never set in the loop, low value will equal high value and will be on the edge of the map */
	unsigned short lowLoadValue = Table->Load[0];
	unsigned short highLoadValue = Table->Load[Table->LoadLength -1];

	unsigned char LoadIndex;
	for(LoadIndex=0;LoadIndex<Table->LoadLength;LoadIndex++){
		if(Table->Load[LoadIndex] < realLoad){
			lowLoadValue = Table->Load[LoadIndex];
			lowLoadIndex = LoadIndex;
		}else if(Table->Load[LoadIndex] > realLoad){
			highLoadValue = Table->Load[LoadIndex];
			highLoadIndex = LoadIndex;
			break;
		}else if(Table->Load[LoadIndex] == realLoad){
			lowLoadValue = Table->Load[LoadIndex];
			highLoadValue = Table->Load[LoadIndex];
			lowLoadIndex = LoadIndex;
			highLoadIndex = LoadIndex;
			break;
		}
	}


	/* Obtain the four corners surrounding the spot of interest */
	unsigned short lowRPMLowLoad = Table->Table[(Table->LoadLength * lowRPMIndex) + lowLoadIndex];
	unsigned short lowRPMHighLoad = Table->Table[(Table->LoadLength * lowRPMIndex) + highLoadIndex];
	unsigned short highRPMLowLoad = Table->Table[(Table->LoadLength * highRPMIndex) + lowLoadIndex];
	unsigned short highRPMHighLoad = Table->Table[(Table->LoadLength * highRPMIndex) + highLoadIndex];


	/* Restore the ram page before doing the math */
	RPAGE = oldRPage;


	/* Find the two side values to interpolate between by interpolation */
	unsigned short lowRPMIntLoad = lowRPMLowLoad + (((signed long)((signed long)lowRPMHighLoad - lowRPMLowLoad) * (realLoad - lowLoadValue))/ (highLoadValue - lowLoadValue));
	unsigned short highRPMIntLoad = highRPMLowLoad + (((signed long)((signed long)highRPMHighLoad - highRPMLowLoad) * (realLoad - lowLoadValue))/ (highLoadValue - lowLoadValue));


	/* Interpolate between the two side values and return the result */
	return lowRPMIntLoad + (((signed long)((signed long)highRPMIntLoad - lowRPMIntLoad) * (realRPM - lowRPMValue))/ (highRPMValue - lowRPMValue));
}


unsigned short lookupTwoDTableUS(twoDTableUS * Table, unsigned short Value){

	/* Find the bounding axis indices, axis values and lookup values */
	unsigned char lowIndex = 0;
	unsigned char highIndex = 15;
	/* If never set in the loop, low value will equal high value and will be on the edge of the map */
	unsigned short lowAxisValue = Table->Axis[0];
	unsigned short highAxisValue = Table->Axis[15];
	unsigned short lowLookupValue = Table->Values[0];
	unsigned short highLookupValue = Table->Values[15];

	unsigned char Index;
	for(Index=0;Index<16;Index++){
		if(Table->Axis[Index] < Value){
			lowIndex = Index;
			lowAxisValue = Table->Axis[Index];
			lowLookupValue = Table->Values[Index];
		}else if(Table->Axis[Index] > Value){
			highIndex = Index;
			highAxisValue = Table->Axis[Index];
			highLookupValue = Table->Values[Index];
			break;
		}else if(Table->Axis[Index] == Value){
			return Table->Values[Index]; // If right on, just return the value
		}
	}


	/* Interpolate and return the value */
	return lowLookupValue + (((signed long)((signed long)highLookupValue - lowLookupValue) * (Value - lowAxisValue))/ (highAxisValue - lowAxisValue));
}


/* Returns 0 on success and error code otherwise */
unsigned short setAxisValue(unsigned short index, unsigned short value, unsigned short axis[], unsigned short length, unsigned short errorBase){
	if(index >= length){
		return errorBase + invalidAxisIndex;
	}else{
		if(index > 0){
			/* Ensure value isn't lower than the one below */
			if(axis[index - 1] > value){
				return errorBase + invalidAxisOrder;
			}
		}
		if(index < (length -1)){
			/* Ensure value isn't higher than the one above */
			if(value > axis[index + 1]){
				return errorBase + invalidAxisOrder;
			}
		}
	}

	/* If we got this far all is well, set the value */
	axis[index] = value;
	return 0;
}


unsigned short setPagedMainTableCellValue(unsigned char RPageValue, mainTable* Table, unsigned short RPMIndex, unsigned short LoadIndex, unsigned short cellValue){
	unsigned char oldRPage = RPAGE;
	unsigned short errorID = 0;
	RPAGE = RPageValue;
	if(RPMIndex < Table->RPMLength){
		if(LoadIndex < Table->LoadLength){
			Table->Table[(Table->LoadLength * RPMIndex) + LoadIndex] = cellValue;
		}else{
			errorID = invalidMainTableLoadIndex;
		}
	}else{
		errorID = invalidMainTableRPMIndex;
	}
	RPAGE = oldRPage;
	return errorID;
}


unsigned short setPagedMainTableRPMValue(unsigned char RPageValue, mainTable* Table, unsigned short RPMIndex, unsigned short RPMValue){
	unsigned char oldRPage = RPAGE;
	RPAGE = RPageValue;
	unsigned short errorID = setAxisValue(RPMIndex, RPMValue, Table->RPM, Table->RPMLength, errorBaseMainTableRPM);
	RPAGE = oldRPage;
	return errorID;
}


unsigned short setPagedMainTableLoadValue(unsigned char RPageValue, mainTable* Table, unsigned short LoadIndex, unsigned short LoadValue){
	unsigned char oldRPage = RPAGE;
	RPAGE = RPageValue;
	unsigned short errorID = setAxisValue(LoadIndex, LoadValue, Table->Load, Table->LoadLength, errorBaseMainTableLoad);
	RPAGE = oldRPage;
	return errorID;
}


unsigned short setPagedTwoDTableCellValue(unsigned char RPageValue, twoDTableUS* Table, unsigned short cellIndex, unsigned short cellValue){
	if(cellIndex > 15){
		return invalidTwoDTableIndex;
	}else{
		unsigned char oldRPage = RPAGE;
		RPAGE = RPageValue;
		Table->Values[cellIndex] = cellValue;
		RPAGE = oldRPage;
		return 0;
	}
}


unsigned short setPagedTwoDTableAxisValue(unsigned char RPageValue, twoDTableUS* Table, unsigned short axisIndex, unsigned short axisValue){
	unsigned char oldRPage = RPAGE;
	RPAGE = RPageValue;
	unsigned short errorID = setAxisValue(axisIndex, axisValue, Table->Axis, 16, errorBaseTwoDTableAxis);
	RPAGE = oldRPage;
	return errorID;
}


/* Check that the configuration of the table is valid. Assumes pages are correctly set. */
unsigned short validateMainTable(mainTable* Table){
	/* If required and only if required extend this to take r and f pages and check	*/
	/* any main table, not just a freshly received untrusted ones in linear space	*/

	if(Table->RPMLength > MAINTABLE_MAX_RPM_LENGTH){
		return invalidMainTableRPMLength;
	}else if(Table->LoadLength > MAINTABLE_MAX_LOAD_LENGTH){
		return invalidMainTableLoadLength;
	}else if((Table->RPMLength * Table->LoadLength) > MAINTABLE_MAX_MAIN_LENGTH){
		return invalidMainTableMainLength;
	}else{
		/* Check the order of the RPM axis */
		unsigned char i;
		for(i=0;i<(Table->RPMLength - 1);i++){
			if(Table->RPM[i] > Table->RPM[i+1]){
				return invalidMainTableRPMOrder;
			}
		}
		/* Check the order of the Load axis */
		unsigned char j;
		for(j=0;j<(Table->LoadLength - 1);j++){
			if(Table->Load[j] > Table->Load[j+1]){
				return invalidMainTableLoadOrder;
			}
		}
		/* If we made it this far all is well */
		return 0;
	}
}


/* Check that the configuration of the table is valid */
unsigned short validateTwoDTable(twoDTableUS* Table){
	/* Check the order of the axis */
	unsigned char i;
	for(i=0;i<(TWODTABLEUS_LENGTH - 1);i++){
		if(Table->Axis[i] > Table->Axis[i+1]){
			return invalidTwoDTableAxisOrder;
		}
	}
	return 0;
}