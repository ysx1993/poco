//
// CompressTest.cpp
//
// $Id: //poco/1.3/Zip/testsuite/src/CompressTest.cpp#5 $
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


#include "CompressTest.h"
#include "ZipTest.h"
#include "Poco/Zip/Compress.h"
#include "Poco/Zip/ZipManipulator.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include <fstream>


using namespace Poco::Zip;


CompressTest::CompressTest(const std::string& name): CppUnit::TestCase(name)
{
}


CompressTest::~CompressTest()
{
}


void CompressTest::testSingleFile()
{
	std::ofstream out("appinf.zip", std::ios::binary);
	Poco::Path theFile(ZipTest::getTestFile("test.zip"));
	Compress c(out, true);
	c.addFile(theFile, theFile.getFileName());
	ZipArchive a(c.close());
}


void CompressTest::testDirectory()
{
	std::ofstream out("pocobin.zip", std::ios::binary);
	Poco::File aFile("some/");
	if (aFile.exists())
		aFile.remove(true);
	Poco::File aDir("some/recursive/dir/");
	aDir.createDirectories();
	Poco::File aDir2("some/other/recursive/dir/");
	aDir2.createDirectories();
	Poco::File aF("some/recursive/dir/test.file");
	aF.createFile();
	Poco::FileOutputStream fos(aF.path());
	fos << "just some test data";
	fos.close();

	Poco::Path theFile(aFile.path());
	theFile.makeDirectory();
	Compress c(out, true);
	c.addRecursive(theFile, ZipCommon::CL_MAXIMUM, false, theFile);
	ZipArchive a(c.close());
}


void CompressTest::testManipulator()
{
	{
		std::ofstream out("appinf.zip", std::ios::binary);
		Poco::Path theFile(ZipTest::getTestFile("test.zip"));
		Compress c(out, true);
		c.addFile(theFile, theFile.getFileName());
		ZipArchive a(c.close());
	}
	ZipManipulator zm("appinf.zip", true);
	zm.renameFile("test.zip", "renamedtest.zip");
	zm.addFile("doc/othertest.zip", ZipTest::getTestFile("test.zip"));
	ZipArchive archive=zm.commit();
	assert (archive.findHeader("doc/othertest.zip") != archive.headerEnd());
}


void CompressTest::testManipulatorDel()
{
	{
		std::ofstream out("appinf.zip", std::ios::binary);
		Poco::Path theFile(ZipTest::getTestFile("test.zip"));
		Compress c(out, true);
		c.addFile(theFile, theFile.getFileName());
		ZipArchive a(c.close());
	}
	ZipManipulator zm("appinf.zip", true);
	zm.deleteFile("test.zip");
	zm.addFile("doc/data.zip", ZipTest::getTestFile("data.zip"));
	ZipArchive archive=zm.commit();
	assert (archive.findHeader("test.zip") == archive.headerEnd());
	assert (archive.findHeader("doc/data.zip") != archive.headerEnd());
}


void CompressTest::testManipulatorReplace()
{
	{
		std::ofstream out("appinf.zip", std::ios::binary);
		Poco::Path theFile(ZipTest::getTestFile("test.zip"));
		Compress c(out, true);
		c.addFile(theFile, theFile.getFileName());
		ZipArchive a(c.close());
	}
	ZipManipulator zm("appinf.zip", true);
	zm.replaceFile("test.zip", ZipTest::getTestFile("doc.zip"));
	
	ZipArchive archive=zm.commit();
	assert (archive.findHeader("test.zip") != archive.headerEnd());
	assert (archive.findHeader("doc.zip") == archive.headerEnd());
}


void CompressTest::setUp()
{
}


void CompressTest::tearDown()
{
}


CppUnit::Test* CompressTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("CompressTest");

	CppUnit_addTest(pSuite, CompressTest, testSingleFile);
	CppUnit_addTest(pSuite, CompressTest, testDirectory);
	CppUnit_addTest(pSuite, CompressTest, testManipulator);
	CppUnit_addTest(pSuite, CompressTest, testManipulatorDel);
	CppUnit_addTest(pSuite, CompressTest, testManipulatorReplace);

	return pSuite;
}
