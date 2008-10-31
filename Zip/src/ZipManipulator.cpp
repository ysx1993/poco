//
// ZipManipulator.cpp
//
// $Id: //poco/1.3/Zip/src/ZipManipulator.cpp#2 $
//
// Library: Zip
// Package: Manipulation
// Module:  ZipManipulator
//
// Copyright (c) 2007, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/Zip/ZipManipulator.h"
#include "Poco/Zip/ZipException.h"
#include "Poco/Zip/ZipUtil.h"
#include "Poco/Zip/Add.h"
#include "Poco/Zip/Delete.h"
#include "Poco/Zip/Keep.h"
#include "Poco/Zip/Rename.h"
#include "Poco/Zip/Replace.h"
#include "Poco/Zip/Compress.h"
#include "Poco/Delegate.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include <fstream>


namespace Poco {
namespace Zip {


ZipManipulator::ZipManipulator(const std::string& zipFile, bool backupOriginalFile):
	_zipFile(zipFile),
	_backupOriginalFile(backupOriginalFile),
	_changes(),
	_in(0)
{
	std::ifstream in(zipFile.c_str());
	_in = new ZipArchive(in);
}


ZipManipulator::~ZipManipulator()
{
}


void ZipManipulator::deleteFile(const std::string& zipPath)
{
	const ZipLocalFileHeader& entry = getForChange(zipPath);
	addOperation(zipPath, new Delete(entry));
}


void ZipManipulator::replaceFile(const std::string& zipPath, const std::string& localPath)
{
	const ZipLocalFileHeader& entry = getForChange(zipPath);
	addOperation(zipPath, new Replace(entry, localPath));
}


void ZipManipulator::renameFile(const std::string& zipPath, const std::string& newZipPath)
{
	const ZipLocalFileHeader& entry = getForChange(zipPath);
	// checked later in Compress too but the earlier one gets the error the better
	std::string fn = ZipUtil::validZipEntryFileName(newZipPath); 
	addOperation(zipPath, new Rename(entry, fn));
}


void ZipManipulator::addFile(const std::string& zipPath, const std::string& localPath, ZipCommon::CompressionMethod cm, ZipCommon::CompressionLevel cl)
{
	addOperation(zipPath, new Add(zipPath, localPath, cm, cl));
}


ZipArchive ZipManipulator::commit()
{
	// write to a tmp file
	std::string outFile(_zipFile + ".tmp");
	ZipArchive retVal(compress(outFile));
	//renaming
	{
		Poco::File aFile(_zipFile);
		if (_backupOriginalFile)
		{
			Poco::File tmp(_zipFile+".bak");
			if (tmp.exists())
				tmp.remove();
			aFile.renameTo(_zipFile+".bak");
		}
		else
			aFile.remove();
	}

	{
		Poco::File resFile(outFile);
		Poco::File zipFile(_zipFile);
		if (zipFile.exists())
			zipFile.remove();
		resFile.renameTo(_zipFile);
	}
	return retVal;
}


const ZipLocalFileHeader& ZipManipulator::getForChange(const std::string& zipPath) const
{
	ZipArchive::FileHeaders::const_iterator it = _in->findHeader(zipPath);
	if (it == _in->headerEnd())
		throw ZipManipulationException("entry not found: " + zipPath);

	if (_changes.find(zipPath) != _changes.end())
		throw ZipManipulationException("A change request exists already for entry " + zipPath);

	return it->second;
}


void ZipManipulator::addOperation(const std::string& zipPath, ZipOperation::Ptr ptrOp)
{
	std::pair<Changes::iterator, bool> result = _changes.insert(std::make_pair(zipPath, ptrOp));
	if (!result.second)
		throw ZipManipulationException("A change request exists already for entry " + zipPath);
}


void ZipManipulator::onEDone(const void*, const ZipLocalFileHeader& hdr)
{
	EDone(this, hdr);
}


ZipArchive ZipManipulator::compress(const std::string& outFile)
{
	// write to a tmp file
	Poco::FileInputStream in(_zipFile);
	Poco::FileOutputStream out(outFile);
	Compress c(out, true);
	c.EDone += Poco::Delegate<ZipManipulator, const ZipLocalFileHeader>(this, &ZipManipulator::onEDone);
	
	ZipArchive::FileHeaders::const_iterator it = _in->headerBegin();
	for (; it != _in->headerEnd(); ++it)
	{
		Changes::iterator itC = _changes.find(it->first);
		if (itC != _changes.end())
		{
			itC->second->execute(c, in);
			_changes.erase(itC);
		}
		else
		{
			Keep k(it->second);
			k.execute(c, in);
		}
	}
	//Remaining files are add operations!
	Changes::iterator itC = _changes.begin();
	for (; itC != _changes.end(); ++itC)
	{
		itC->second->execute(c, in);
	}
	_changes.clear();
	c.EDone -= Poco::Delegate<ZipManipulator, const ZipLocalFileHeader>(this, &ZipManipulator::onEDone);
	in.close();
	return c.close();
}


} } // namespace Poco::Zip
