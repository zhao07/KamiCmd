#ifndef FILE_COPY_H
#define FILE_COPY_H

#include <QtCore/QMutex>
#include <QtCore/QList>
#include <QtCore/QAtomicInt>
#include <QtGui/QDialog>

#include "IFileCopy.h"
#include "IFileSystem.h"
#include "library.h"

#include "FilesToCopy.h"


class FileCopy : public IFileCopy
{
	Q_OBJECT

public:
	FileCopy(QObject *parent = 0);
	~FileCopy();

	void PrepareForCopy(const IFilesToCopy & files);

	const QString GetFileName() const;
	const QString & GetDestination() const;

	bool Exec();
	void Pause();
	void Resume();
	void Cancel();

	qint64 GetTotalSize() const;
	int GetCurrentFileProgress() const;
	const FileInfo *GetCurrentCopiedFile() const;
	qint64 GetCurrentFileBytesCopied() const;
	qint64 GetTotalBytesCopied() const;
	OperationState GetState() const;
	QString GetType() const;
	int GetProgress() const;
	int GetCurentFileNumber() const;
	
	virtual void ShowProgressDialog(QWidget *parent = 0);

	void SetErrorHandling(QFile::FileError error, ErrorHandling handling);
	FileCopy::ErrorHandling GetErrorHandling(QFile::FileError error) const;

signals:
	void reportError(const QString filePath, QFile::FileError error, const QString errorText);

private:
	enum ErrorProcessingDesicion
	{
		Stop, Continue, Retry
	};

	void copyFile(const QString & from, const QString & to);
	bool copyMemory(const uchar *src, uchar *dst, int offset, int size);
	ErrorProcessingDesicion processFileError(const QFile & file);
	

	ErrorHandling errorHandling[15]; // this array describes behaviour if some error from QFile::FileError happens

	QMutex stateMutex;
	OperationState state;
	QMutex pauseMutex;

	const CopiedFile *currentCopiedFile;
	IFileSystem *fileSystem;
	QDir destinationDirectory;
	
	const IFilesToCopy *filesToCopy;
	int currentFileIndex;
	int currentFileBytesCopied;
	qint64 bytesCopied;
	
};

#endif // FILE_COPY
