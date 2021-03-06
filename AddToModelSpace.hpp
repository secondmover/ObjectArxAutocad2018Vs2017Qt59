﻿#ifndef ADDTOMODELSPACE_HPP
#define ADDTOMODELSPACE_HPP

#include <Final.object_arx_global.hpp>

namespace sstd {

	namespace _add_to_model_space_private {
		
		template<typename T>
		inline AcDbObjectId addToModelSpace(const ArxClosePointer<T> & arg,
			ArxClosePointer<AcDbBlockTableRecord>&argTR) {
			AcDbObjectId varAns;
			if (argTR->appendAcDbEntity(varAns, arg.pointer())!=Acad::eOk) {
				return AcDbObjectId::kNull;
			}
			return varAns;
		}

		inline bool getCurrentBlockTable(ArxClosePointer<AcDbBlockTable>& argT,
			ArxClosePointer<AcDbBlockTableRecord> & argTR) {
			auto varAHAS = acdbHostApplicationServices();
			if (nullptr == varAHAS) { return false; }
			auto varWDB = varAHAS->workingDatabase();
			if (nullptr == varWDB) { return false; }
			if (varWDB->getBlockTable(argT, AcDb::kForRead) != Acad::eOk) {
				return false;
			}
			if (argT->getAt(ACDB_MODEL_SPACE, argTR, AcDb::kForWrite) != Acad::eOk) {
				return false;
			}
			return true;
		}

	}	

	template<typename T>
	inline AcDbObjectId addToModelSpace(const ArxClosePointer<T> & varItem) {
		ArxClosePointer<AcDbBlockTable> varBlockTable;
		ArxClosePointer<AcDbBlockTableRecord> varBlockTableRecord;
		if (_add_to_model_space_private::getCurrentBlockTable(
			varBlockTable, varBlockTableRecord)) {
			return _add_to_model_space_private::addToModelSpace(varItem,
				varBlockTableRecord);
		}
		return AcDbObjectId::kNull;
	}

	template<typename T>
	inline AcDbObjectId addToModelSpace(T * arg) {
		const ArxClosePointer<T> varItem{ arg };
		return addToModelSpace(varItem);
	}

	template<typename T,typename U>
	inline void addToModelSpace(T b,const U & e){
		ArxClosePointer<AcDbBlockTable> varBlockTable;
		ArxClosePointer<AcDbBlockTableRecord> varBlockTableRecord;
		if (_add_to_model_space_private::getCurrentBlockTable(
			varBlockTable, varBlockTableRecord)) {
			for (; b != e; ++b) {
				_add_to_model_space_private::addToModelSpace(*b,
					varBlockTableRecord);
			}
		}
	}

}/*namespace sstd*/

#endif // ADDTOMODELSPACE_HPP

