#ifndef FS_INTERFACE_H
#define FS_INTERFACE_H

#include <QtPlugin>
#include <QDateTime>
#include <QFile>
#include <QtGui/QIcon>
#include <QMetaType>

struct FileInfo
{
public:

	enum Attributes{
		Archive		= 0x001,
		Compressed	= 0x002,
		Directory	= 0x004,
		Encrypted	= 0x008,
		Hidden		= 0x010,
		Normal		= 0x020,
		Offline		= 0x040,
		ReadOnly	= 0x080,
		ReparsePoint= 0x100,
		SparseFile	= 0x200,
		System		= 0x400,
		Temporary	= 0x800,
		UpOneLevel  = 0xA00
	};

	quint32 attributes;
	qint64 size;
	
	QDateTime creationTime;
	QDateTime lastAccessTime;
	QDateTime lastWriteTime;
	
	QString path; // e.g. "C:/Temp/"
	QString name; // e.g. "File.txt"
	QString alternateName; // name in 8.3 format

	QIcon icon;

	void init()
	{
		attributes = 0;
		size = 0;
	};

	QString FilePath() const
	{
		return path + name;
	}
};
Q_DECLARE_METATYPE(FileInfo)

class IFileSystem
{
public:
	virtual ~IFileSystem()	{}
	virtual int SetPath(QString path) = 0; // return value is: 0 = failed, 1 = success, -1 = partial success
	virtual QString GetPath() = 0;
	virtual uint GetNumberOfFiles() = 0;
	virtual int  UpOneLevel() = 0; // return value is: 0 = failed, 1 = success, -1 = partial success
	virtual bool isRoot() = 0;
	
	virtual QFile* GetFile(QString path) = 0;
	virtual bool GetFirstFileInfo(FileInfo &info) = 0;
	virtual bool GetNextFileInfo(FileInfo &info) = 0;
};

#endif // FS_INTERFACE_H
