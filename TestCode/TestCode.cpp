﻿#include "TestCode.hpp"
#include <acedCmdNF.h>
//AcApCommandLineEditor

namespace sstd {

	extern void loadTestCode() { TestCode::load(); }

	TestCode::TestCode() {

	}

	void TestCode::load() {
		arx_add_main_command<TestCode>();
	}

	namespace {
		inline Acad::ErrorStatus openFile(const std::wstring_view & argFileName) {
			
			Acad::ErrorStatus varE =
				acDocManager->appContextOpenDocument(argFileName.data());

			if (varE != Acad::eOk) {
				acutPrintf(LR"(打开文件失败:)");
				acutPrintf(acadErrorStatusText(varE));
				acutPrintf(LR"(:)");
				acutPrintf(argFileName.data());
				acutPrintf(LR"(
)");
			}
			return varE;
		}

			
				 

	}/*namespace*/

	void TestCode::main() {

		{
			auto db = acdbHostApplicationServices()->workingDatabase();
			//acedGetCurrentUCS
			//acedSetCurrentUCS
			//AcGeMatrix3d
			//acdbUcs2Wcs
			(void)db;
		}

		const auto varStartTime = std::chrono::high_resolution_clock::now();

		auto varCurrentDocument = acDocManager->curDocument();
		const static constexpr auto varFileName = LR"(D:\test1\drawing2.dwg)"sv;
		
		bool varIsOk = false;
		acDocManager->executeInApplicationContext([](void * argIsOk) {
			*reinterpret_cast<bool*>(argIsOk) = (Acad::eOk == openFile(varFileName.data()));
		},&varIsOk);

		if (varIsOk) {

			std::unique_ptr<AcApDocumentIterator> varIt{ acDocManager->newAcApDocumentIterator() };

			for (;!varIt->done();varIt->step() ) {
				auto varDoc = varIt->document();
				if ((varDoc==nullptr)||(varDoc == varCurrentDocument)) { continue; }

				acDocManager->sendStringToExecute(varDoc,LR"(circle
0,0,0
100.58
)"
					LR"(qsave
)"
					LR"(close
)"
				,false);				

			}
			
			const auto varLength = std::chrono::high_resolution_clock::now() - varStartTime;
			const auto varLengthW = QString::number(std::chrono::duration_cast<std::chrono::duration<double>>(varLength).count())
				.toStdWString();
			acutPrintf(LR"(用时:)");
			acutPrintf(varLengthW.c_str());

		}
				
	}

}/*namespace sstd*/


/**
https://my.oschina.net/u/2930533/blog/1618956
void ZffCHAP4AddEntInUcs()
{
// 转换坐标系的标记
struct resbuf wcs, ucs;
wcs.restype = RTSHORT;
wcs.resval.rint = 0;
ucs.restype = RTSHORT;
ucs.resval.rint = 1;

// 提示用户输入直线的起点和终点
ads_point pt1, pt2;
if (acedGetPoint(NULL, "拾取直线的起点：", pt1) != RTNORM)
return;
if (acedGetPoint(pt1, "拾取直线的终点：", pt2) != RTNORM)
return;

// 将起点和终点坐标转换到WCS
acedTrans(pt1, &ucs, &wcs, 0, pt1);
acedTrans(pt2, &ucs, &wcs, 0, pt2);

// 创建直线
AcDbLine *pLine = new AcDbLine(asPnt3d(pt1), asPnt3d(pt2));
AcDbBlockTable *pBlkTbl = NULL;
acdbHostApplicationServices()->workingDatabase()->getBlockTable(pBlkTbl, AcDb::kForRead);
AcDbBlockTableRecord *pBlkTblRcd= NULL;
pBlkTbl->getAt(ACDB_MODEL_SPACE, pBlkTblRcd,AcDb::kForWrite);
pBlkTbl->close();
pBlkTblRcd->appendAcDbEntity(pLine);
pLine->close();
pBlkTblRcd->close();
}

acdbWcs2Ucs
acdbUcs2Wcs

**/


