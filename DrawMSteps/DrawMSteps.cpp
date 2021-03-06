﻿#include <QtQml/QtQml>
#include <object_arx_global.hpp>
#include <vector>
#include <array>
#include <optional>
#include "../ThirdPart/ADN/ADNAssocCreateConstraint.hpp"
#include "DrawMSteps.hpp"

namespace {
	inline constexpr const wchar_t * const _this_layerName() {
		return LR"(参考线)";
	}
}

namespace sstd {
	extern std::array<std::uint8_t, 3> randColr();
	extern AcCmColor smallChange(const AcCmColor & );
}

namespace {
	inline auto add_scale_half(const double & a, const double &b) {
		return (a + b)*0.5;
	}

	inline auto mid(const AcGePoint3d & x, const AcGePoint3d &y) {
		return AcGePoint3d(add_scale_half(x.x, y.x),
			add_scale_half(x.y, y.y),
			0.
		);
	}
}

sstd::DrawMSteps::DrawMSteps() {

}

void sstd::DrawMSteps::load() {
	arx_add_main_command<DrawMSteps>();
}

namespace sstd {
	extern void loadDrawMStep() {
		DrawMSteps::load();
	}
}/*namespace sstd*/

namespace sstd {
	extern void UCS2WCS(const double * i, double *o);
}

namespace {

	static inline AcGePoint3d UcsToWorld(const AcGePoint3d& ucsPoint) {
		AcGePoint3d varAns;
		sstd::UCS2WCS(&ucsPoint.x, &varAns.x);
		return varAns;
	}

	class DrawLayerLock {
		AcDbDatabase * d;
		AcDbObjectId l;
	public:
		inline DrawLayerLock(AcDbDatabase * a) :d(a) {
			l = d->clayer();
			d->setClayer(d->layerZero());
		}
		inline ~DrawLayerLock() {
			d->setClayer(l);
		}
	};

	class RValue {
	public:
		double value = 0.;
		bool isDx = false;
	};

	class FileReader {
	public:
		QString $FileName;
		QString $FileData;
		std::vector<RValue> $Result;
		inline bool read(const QString & varFileName) try {
			$Result.clear();
			$FileName = varFileName;
			if ($FileName.isEmpty()) { svthrow(LR"(无文件名)"sv); }
			{
				{
					QFile varF{ $FileName };
					if (false == varF.open(QFile::ReadOnly)) {
						svthrow(LR"(打开文件失败)"sv);
					}
					{
						QTextStream varStream{ &varF };
						$FileData = varStream.readAll();
						if ($FileData.isEmpty()) {
							svthrow(LR"(文件内容为空)"sv);
						}
					}
				}
				QTextStream varStream{ &$FileData };
				enum class RunType :int {
					X, Y, J, None
				};
				RunType varType = RunType::None;
				QJSEngine varJSE;
				RValue varRValue;
				while (varStream.atEnd() == false) {
					const QString varLine = varStream
						.readLine()
						.trimmed();

					if (varLine.isEmpty()) {
						continue;
					}

					if (varLine.startsWith(QChar(':'))) {
						varType = RunType::None;

						if (varLine.startsWith(QStringLiteral(R"(:x)") , Qt::CaseInsensitive)) {
							varType = RunType::X;
							continue;
						}
						else if (varLine.startsWith(QStringLiteral(R"(:y)"), Qt::CaseInsensitive)) {
							varType = RunType::Y;
							continue;
						}
						else if (varLine.startsWith(QStringLiteral(R"(:j)"), Qt::CaseInsensitive)) {
							varType = RunType::J;
							continue;
						}

					}

					switch (varType) {
					case RunType::X: {
						const auto varV = varJSE.evaluate(varLine).toNumber();
						varRValue.isDx = true;
						varRValue.value = varV;
						$Result.push_back(varRValue);
					}break;
					case RunType::Y: {
						const auto varV = varJSE.evaluate(varLine).toNumber();
						varRValue.isDx = false;
						varRValue.value = varV;
						$Result.push_back(varRValue);
					}break;
					case RunType::J: {
						varJSE.evaluate(varLine);
					}break;
					default:continue;
					}

				}/*while*/

			}
			if ($Result.empty()) {
				svthrow(LR"(文件内容为空)"sv);
			}
			return true;
		}
		catch (...) { return false; }
	};

	class Main {
	public:
		AcDbDatabase * $DB;
		std::unique_ptr<FileReader> $FileReader;
		QString $FileName;
		AcGePoint3d $StartPoint;
		class VirtualLine {
		public:
			AcDbObjectId $ID;
			AcGePoint3d $StartPoint;
			AcGePoint3d $EndPoint;
			AcGePoint3d $MidPoint;
			double $Length = 0.;
			bool isDx = false;
		};
		std::vector< VirtualLine > $VirtualLines;
		std::vector< VirtualLine > $MirVirtualLines;
		inline void construct() {
			_p_get_point();
			_p_get_file_mane();
		}
		inline void run() {
			$FileReader = std::unique_ptr<FileReader>{
				new FileReader
			};

			if ($FileReader->read($FileName) == false) {
				svthrow(LR"(文件内容为空)"sv);
			}

			_make_virtual_lines();
			_p_make_mirror_virtual_lines();

			_draw_();
			_draw_mirror();

			_p_constraint();
			_p_constraint_mirror();

		}
	private:
		inline void _p_constraint_mirror() {
			/*{
				AcGeMatrix3d ucs;
				acedGetCurrentUCS(ucs);
				for (auto & varI : $MirVirtualLines) {
					varI.$EndPoint = UcsToWorld(varI.$EndPoint, ucs);
					varI.$MidPoint = UcsToWorld(varI.$MidPoint, ucs);
					varI.$StartPoint = UcsToWorld(varI.$StartPoint, ucs);
				}
			}*/

			/*增加水平垂直约束*/
			for (auto & varI : $MirVirtualLines) {
				if (varI.isDx) {
					AcDbAssoc2dConstraintAPI::createHorizontalConstraint(varI.$ID, varI.$MidPoint);
				}
				else {
					AcDbAssoc2dConstraintAPI::createVerticalConstraint(varI.$ID, varI.$MidPoint);
				}
			}
			/*增加重合约束*/
			{
				const auto varE = $MirVirtualLines.end();
				auto varP = $MirVirtualLines.begin();
				auto varL = varP + 1;
				for (; varL != varE; varP = varL++) {
					AcDbAssoc2dConstraintAPI::createCoincidentConstraint(varP->$ID, varL->$ID, varP->$EndPoint, varP->$EndPoint);
				}
			}

			{
				/*增加镜像相关约束*/
				AcDbAssoc2dConstraintAPI::createCoincidentConstraint($MirVirtualLines[0].$ID, $VirtualLines[0].$ID,
					$MirVirtualLines[0].$StartPoint, $VirtualLines[0].$StartPoint);
				auto varMB = $MirVirtualLines.begin();
				const auto varME = $MirVirtualLines.end();
				auto varB = $VirtualLines.begin();
				for (; varMB != varME; ++varMB, ++varB) {
					AcDbAssoc2dConstraintAPI::createEqualLengthConstraint(
						varMB->$ID, varB->$ID, varMB->$MidPoint, varB->$MidPoint
					);
				}

			}
		}

		inline void _draw_mirror() {
			sstd::ArxClosePointer<AcDbBlockTable> varBlockTable;
			sstd::ArxClosePointer<AcDbBlockTableRecord> varBlockTableRecord;
			std::vector< sstd::ArxClosePointer< AcDbLine > > varLines;
			varLines.reserve($VirtualLines.size());
			{
				auto varE = $DB->getBlockTable(varBlockTable, AcDb::kForRead);
				if (varE != eOk) { svthrow(LR"(打开BlockTable失败)"sv); }
				varE = varBlockTable->getAt(ACDB_MODEL_SPACE,
					varBlockTableRecord,
					AcDb::kForWrite);
				if (varE != eOk) { svthrow(LR"(打开模型空间失败)"sv); }
			}

			{
				AcCmColor varColor;
				const auto varRColor = sstd::randColr();
				varColor.setRGB(varRColor[0], varRColor[1], varRColor[2]);
				for (auto & varI : $MirVirtualLines) {
					auto & varV = varLines.emplace_back(new AcDbLine{ varI.$StartPoint,varI.$EndPoint });
					varBlockTableRecord->appendAcDbEntity(varI.$ID, varV);
					varV->setLayer(_this_layerName());
					/**set object color****************************/
					varV->setColor(sstd::smallChange(varColor));
				}
			}
		}

		inline void _p_make_mirror_virtual_lines() {
			$MirVirtualLines.clear();
			$MirVirtualLines.reserve($VirtualLines.size());
			auto varB = $VirtualLines.cbegin();
			const auto varSPoint = varB->$StartPoint;
			std::unique_ptr<AcGePlane>  varMirrorPlane;
			const bool isMirrorH = varB->isDx ? false : true;
			if (isMirrorH) {
				varMirrorPlane.reset(new AcGePlane{ varSPoint, AcGeVector3d{ 0,1,0 } });
			}
			else {
				varMirrorPlane.reset(new AcGePlane{ varSPoint, AcGeVector3d{ 1,0,0 } });
			}
			const auto varE = $VirtualLines.cend();
			for (; varB != varE; ++varB) {
				auto & varI = $MirVirtualLines.emplace_back();
				varI.$EndPoint = varB->$EndPoint;
				varI.$EndPoint.mirror(*varMirrorPlane);
				varI.$StartPoint = varB->$StartPoint;
				varI.$StartPoint.mirror(*varMirrorPlane);
				varI.$MidPoint = varB->$MidPoint;
				varI.$MidPoint.mirror(*varMirrorPlane);
				varI.isDx = varB->isDx;
			}

			/*{
				QFile varLogFile{ QString::fromUtf8(u8R"(C:\Users\b\Desktop\test\log2.txt)") };
				varLogFile.open(QIODevice::WriteOnly);
				QTextStream varStream{ &varLogFile };

				for (auto & varI : $MirVirtualLines) {

					varStream << " sx:" << varI.$StartPoint.x
						<< " sy:" << varI.$StartPoint.y
						<< " sz:" << varI.$StartPoint.z;
					varStream << " mx:" << varI.$MidPoint.x
						<< " my:" << varI.$MidPoint.y
						<< " mz:" << varI.$MidPoint.z;
					varStream << " ex:" << varI.$EndPoint.x
						<< " ey:" << varI.$EndPoint.y
						<< " ez:" << varI.$EndPoint.z;
					varStream << endl;
				}
			}*/

		}

		inline void _p_constraint() {
			/*{
				AcGeMatrix3d ucs;
				acedGetCurrentUCS(ucs);
				for (auto & varI : $VirtualLines) {
					varI.$EndPoint = UcsToWorld(varI.$EndPoint, ucs);
					varI.$MidPoint = UcsToWorld(varI.$MidPoint, ucs);
					varI.$StartPoint = UcsToWorld(varI.$StartPoint, ucs);
				}
			}*/
			/*增加水平垂直约束*/
			for (auto & varI : $VirtualLines) {
				if (varI.isDx) {
					AcDbAssoc2dConstraintAPI::createHorizontalConstraint(varI.$ID, varI.$MidPoint);
				}
				else {
					AcDbAssoc2dConstraintAPI::createVerticalConstraint(varI.$ID, varI.$MidPoint);
				}
			}
			/*增加重合约束*/
			{
				const auto varE = $VirtualLines.end();
				auto varP = $VirtualLines.begin();
				auto varL = varP + 1;
				for (; varL != varE; varP = varL++) {
					AcDbAssoc2dConstraintAPI::createCoincidentConstraint(varP->$ID, varL->$ID, varP->$EndPoint, varP->$EndPoint);
				}
			}
			/*增加水平，垂直尺寸约束*/
			{
				AcDbObjectId varTmp;
				AcGePoint3d varCPos;
				const auto varE = $VirtualLines.end();
				auto varP = $VirtualLines.begin();
				auto varSLineID = varP->$ID;
				auto varSPoint = varP->$StartPoint;
				auto varL = varP + 1;
				for (; varL != varE; varP = varL++) {
					if (varL->isDx) {/*垂直尺寸约束*/
						varCPos = mid(varL->$MidPoint, varSPoint);
						AcDbAssoc2dConstraintAPI::createVerticalDimConstraint(
							varL->$ID, varSLineID,
							varL->$MidPoint, varSPoint,
							varCPos, varTmp
						);
						{
							AcString varName;
							AcDbEvalVariant varValue;
							AcString varExpression;
							AcString varEvaluatorId;
							AcString varErrorString;
							/*getVariableValue*/
							AcDbAssoc2dConstraintAPI::getVariableValue(
								varTmp,
								varName,
								varValue,
								varExpression,
								varEvaluatorId
							);
							/**/
							{
								const QString Expression_ = QString::number(std::abs(
									varSPoint.y - (varL->$StartPoint.y))*2.) +
									QLatin1String(R"(/2)", 2);
								const auto varTS = Expression_.toStdWString();
								varExpression = AcString{ varTS.c_str() };
								//acutPrintf(varTS.c_str());
								//acutPrintf(L"\n");
							}
							/*setVariableValue*/
							AcDbAssoc2dConstraintAPI::setVariableValue(varTmp,
								varValue,
								varExpression,
								varEvaluatorId,
								varErrorString);
						}
					}
					else {/*水平尺寸约束*/
						varCPos = mid(varL->$MidPoint, varSPoint);
						AcDbAssoc2dConstraintAPI::createHorizontalDimConstraint(
							varSLineID, varL->$ID,
							varSPoint, varL->$MidPoint,
							varCPos, varTmp
						);
					}
				}
			}
		}

		inline void _draw_() {
			sstd::ArxClosePointer<AcDbBlockTable> varBlockTable;
			sstd::ArxClosePointer<AcDbBlockTableRecord> varBlockTableRecord;
			std::vector< sstd::ArxClosePointer< AcDbLine > > varLines;
			varLines.reserve($VirtualLines.size());

			{
				auto varE = $DB->getBlockTable(varBlockTable, AcDb::kForRead);
				if (varE != eOk) { svthrow(LR"(打开BlockTable失败)"sv); }
				varE = varBlockTable->getAt(ACDB_MODEL_SPACE,
					varBlockTableRecord,
					AcDb::kForWrite);
				if (varE != eOk) { svthrow(LR"(打开模型空间失败)"sv); }
			}

			{
				AcCmColor varColor;
				const auto varRColor = sstd::randColr();
				varColor.setRGB(varRColor[0], varRColor[1], varRColor[2]);
				for (auto & varI : $VirtualLines) {
					auto & varV = varLines.emplace_back(new AcDbLine{ varI.$StartPoint,varI.$EndPoint });
					varBlockTableRecord->appendAcDbEntity(varI.$ID, varV);
					varV->setLayer(_this_layerName());
					/**set object color****************************/
					varV->setColor( sstd::smallChange(varColor) );
				}
			}

		}

		inline void _make_virtual_lines() {
			$VirtualLines.clear();
			if (bool($FileReader) == false) { svthrow(LR"(打开文件失败)"sv); }
			$VirtualLines.reserve($FileReader->$Result.size());

			{
				/*更新EndPoints*/
				auto varPrePos = $FileReader->$Result.begin();
				auto varEndPos = $FileReader->$Result.end();
				for (; varPrePos != varEndPos; ++varPrePos) {
					auto & varValue = $VirtualLines.emplace_back();
					varValue.$StartPoint = this->$StartPoint;
					if (varPrePos->isDx) {
						varValue.$EndPoint = varValue.$StartPoint;
						varValue.$EndPoint.x += varPrePos->value;
						varValue.isDx = true;
						varValue.$Length = std::abs(varPrePos->value);
					}
					else {
						varValue.$EndPoint = varValue.$StartPoint;
						varValue.$Length = 0.5*varPrePos->value;
						varValue.$EndPoint.y += varValue.$Length;
						varValue.$Length = std::abs(varPrePos->value);
						varValue.isDx = false;
					}
				}
			}

			{
				/*更新StartPoints*/
				auto varPrePos = $VirtualLines.begin();
				auto varLastPos = varPrePos + 1;
				const auto varEndPos = $VirtualLines.end();
				for (; varLastPos != varEndPos; varPrePos = varLastPos++) {
					varLastPos->$StartPoint = varPrePos->$EndPoint;
					if (varLastPos->isDx) {
						varLastPos->$EndPoint.y = varPrePos->$EndPoint.y;
					}
					else {
						varLastPos->$EndPoint.x = varPrePos->$EndPoint.x;
					}
				}
			}
			/*更新MidPoints*/
			for (auto & varI : $VirtualLines) {
				if (varI.isDx) {
					varI.$MidPoint = varI.$StartPoint;
					varI.$MidPoint.x = (varI.$EndPoint.x + varI.$StartPoint.x)*0.5;
				}
				else {
					varI.$MidPoint = varI.$StartPoint;
					varI.$MidPoint.y = (varI.$EndPoint.y + varI.$StartPoint.y)*0.5;
				}
			}

			/*{
				QFile varLogFile{ QString::fromUtf8(u8R"(C:\Users\b\Desktop\test\log1.txt)") };
				varLogFile.open(QIODevice::WriteOnly);
				QTextStream varStream{ &varLogFile };

				for (auto & varI : $VirtualLines) {

					varStream << " sx:" << varI.$StartPoint.x
						<< " sy:" << varI.$StartPoint.y
						<< " sz:" << varI.$StartPoint.z;
					varStream << " mx:" << varI.$MidPoint.x
						<< " my:" << varI.$MidPoint.y
						<< " mz:" << varI.$MidPoint.z;
					varStream << " ex:" << varI.$EndPoint.x
						<< " ey:" << varI.$EndPoint.y
						<< " ez:" << varI.$EndPoint.z;
					varStream << endl;
				}
			}*/
		}

		inline void _p_get_point() {
			const auto varError =
				acedGetPoint(nullptr, LR"(请输入起始中心<0,0>：)", &($StartPoint.x));

			if (RTNONE == varError) {
				$StartPoint = { 0.0,0.0,0.0 };
			}
			else {
				if (varError != RTNORM) {
					svthrow(LR"(获得点失败)"sv);
				}
			}

			$StartPoint = UcsToWorld($StartPoint);

		}

		inline void _p_get_file_mane() {
			QString varFileNameQ;
			{
				const wchar_t * varFileName
					= acDocManager
					->mdiActiveDocument()
					->fileName();
				if (varFileName == nullptr) { svthrow(LR"(获得文件名失败)"sv); }
				varFileNameQ = QString::fromWCharArray(varFileName);
				 
			}
			const auto varPath = QFileInfo(varFileNameQ).absolutePath();
			{
				QDir varDir{ varPath };
				auto varFullFilePath = varDir.absoluteFilePath("xy.wh.txt");
				if ( QFile::exists(varFullFilePath) ) {
					$FileName = varFullFilePath;
					return;
				}
			}
			$FileName = QFileDialog::getOpenFileName(nullptr, {}, varPath);
			if ($FileName.isEmpty()) { svthrow(LR"(获得文件名失败)"sv); }

		}
	};

}/**/

void sstd::DrawMSteps::main() try {
	auto varDB = acdbHostApplicationServices()->workingDatabase();
	DrawLayerLock varThisLock{ varDB };
	QtApplication varQtApp;
	std::unique_ptr<Main> varMain{ new Main };
	varMain->$DB = varDB;
	varMain->construct();
	varMain->run();
}
catch (...) {
	return;
}

