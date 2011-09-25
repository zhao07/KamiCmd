#include "FileCopy.h"
#include "IFileSystem.h"

//#include <QTest>
#include <QDir>
#include <QMessageBox>

FileCopy::FileCopy( QObject *parent /*= 0*/ ) 
	: QObject(parent), bytesCopied(0), state(Running), currentFileIndex(0), currentCopiedFile(0), currentFileBytesCopied(0)
{
	fileSystem = dynamic_cast<IFileSystem *>(g_Core->QueryModule("FS", 1));
	if(!fileSystem)
		g_Core->DebugWrite("FileCopy", "Can't query File system");
}

FileCopy::~FileCopy()
{

}

void FileCopy::PrepareForCopy( const FilesToCopy & files )
{
	filesToCopy = files;
	if(!QFile::exists(files.GetDestination()))
		destinationDirectory.mkpath(files.GetDestination());
	destinationDirectory.setCurrent(files.GetDestination());
}

bool FileCopy::Exec()
{

	for( ; currentFileIndex < filesToCopy.Count() && GetState() != Canceled; ++currentFileIndex)
	{
	//	currentCopiedFileMutex.lock();
		currentCopiedFile = &filesToCopy.GetNextFile();

	//	currentCopiedFileMutex.unlock();
	
		if(!destinationDirectory.exists(currentCopiedFile->RelativePath()))
			destinationDirectory.mkpath(currentCopiedFile->RelativePath());
		QString path = filesToCopy.GetDestination() + currentCopiedFile->RelativePath();
		copyFile(currentCopiedFile->GetFile().path + currentCopiedFile->GetFile().name,
				 path + currentCopiedFile->GetFile().name);
	}
	
//	currentCopiedFileMutex.lock();
	currentCopiedFile = NULL;
//	currentCopiedFileMutex.unlock();

	stateMutex.lock();
	if(state != Canceled)
		state = Finished;
	stateMutex.unlock();
	
	return true;
}

void FileCopy::Pause()
{
	QMutexLocker locker(&stateMutex);
	if(state != Paused && state != Error && state != Canceled)
	{
		state = Paused;
		pauseMutex.lock();
	}
}

void FileCopy::Resume()
{
	QMutexLocker locker(&stateMutex);
	if(state == Paused && state != Canceled && state != Finished)
	{
		state = Running;
		qDebug() << "State changed from " << state << "to Running";
		pauseMutex.unlock();
	}
}

void FileCopy::Cancel()
{
	QMutexLocker locker(&stateMutex);
	if(state == Paused)
		pauseMutex.unlock();
	state = Canceled;
}

FileCopy::OperationState FileCopy::GetState() const
{
	QMutexLocker locker(const_cast<QMutex *>(&stateMutex));
	return state;
}

QString FileCopy::GetType() const
{
	return tr("Copy");
}

int FileCopy::GetProgress() const
{
	qint64 size = filesToCopy.GetTotalSize();
	int percentage = !size ? 100 : bytesCopied / (size / 100.0);
	return percentage;
}

int FileCopy::GetCurrentFileProgress() const
{
//	currentCopiedFileMutex.lock();
	const CopiedFile *file = currentCopiedFile;
//	currentCopiedFileMutex.unlock();
		
	int percentage = (!file || !file->GetFile().size) ? 100 : currentFileBytesCopied / (file->GetFile().size / 100.0);
	return percentage;
}

void FileCopy::copyFile( const QString & from, const QString & to )
{
	currentFileBytesCopied = 0;
	QFile sourceFile(from);
	if(!sourceFile.open(QFile::ReadOnly))
	{
		qDebug() << "Error" <<  "File"  << from  << "can't be opened" << sourceFile.errorString();
		processFileError(sourceFile);
		return;
	}

	const int size = sourceFile.size();

	QFile destinationFile(to);
	if(!destinationFile.open(QFile::ReadWrite) || !destinationFile.resize(size))
	{
		qDebug() << tr("Error") << destinationFile.errorString();
		return;
	}
	
	const uchar *source = sourceFile.map(0, size - 1);
	uchar *destination = destinationFile.map(0, size - 1);

	if(!source || !destination)
	{ 
		qDebug() << "Error" << "Can't copy file " << sourceFile.fileName();
		return;		
	}
	bool r = copyMemory(source, destination, 0, size);
	
	destinationFile.unmap(destination);
	sourceFile.unmap(const_cast<uchar *>(source));
	destinationFile.close();
	sourceFile.close();

	if(GetState() == Canceled && !r)
		destinationFile.remove();
}

void FileCopy::processFileError( const QFile & file )
{
// 	int e = file.error();
// 	if(e == QFile::OpenError || e == QFile::PermissionsError || e == QFile::ReadError || e == QFile::FatalError)
// 		emit reportError(file.error(), file.errorString());
// 	}


}

const QString FileCopy::GetFileName() const
{
//	currentCopiedFileMutex.lock();
	const CopiedFile *file = currentCopiedFile ;
	const QString fileName = file ? file->GetFile().name : QString();
//	currentCopiedFileMutex.unlock();

	return fileName;
}

const QString & FileCopy::GetDestination() const
{
	return filesToCopy.GetDestination();
}

qint64 FileCopy::GetTotalSize() const
{
	return filesToCopy.GetTotalSize();
}

bool FileCopy::copyMemory( const uchar *src, uchar *dst, int offset, int size )
{
	currentFileBytesCopied = offset;
	static const int chunkSize = 4096;

	const int steps = (size - offset) / chunkSize;
	const int remainderBytes = (size - offset) % chunkSize;
	
	dst += offset;
	src += offset;

	for(int i = 0; i < steps; i++)
	{
		if(state == Paused)
		{
			pauseMutex.lock();
			pauseMutex.unlock();
		}
		
		if(state == Canceled)
			return false;
	
		memcpy(dst + i * chunkSize, src + i*chunkSize, chunkSize);
		bytesCopied += chunkSize;
		currentFileBytesCopied += chunkSize;
	}

	memcpy(dst + steps * chunkSize, src + steps * chunkSize, remainderBytes);
	currentFileBytesCopied += remainderBytes;
	bytesCopied += remainderBytes;
	
	return true;
}

qint64 FileCopy::GetCurrentFileBytesCopied() const
{
	return currentFileBytesCopied;
}

qint64 FileCopy::GetTotalBytesCopied() const
{
	return bytesCopied;
}

const FileInfo * FileCopy::GetCurrentCopiedFile()
{
//	currentCopiedFileMutex.lock();
	const CopiedFile *file = currentCopiedFile;
//	currentCopiedFileMutex.unlock();
	if(file)
		return &file->GetFile();
	return NULL;
}

