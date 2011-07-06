#include "library.h"
#include "../IUnitManager.h"
#include "../Unit_TextEditor/ILexerPlugin.h"
class Unit_Lexers : public IUnit
{
    Q_OBJECT
};

ICoreFunctions *g_Core;

std::vector<ILexer *> & getLexers();

QVector<Module *> Lexers_Library::ListModulesAndGetCore( ICoreFunctions *core )
{
	g_Core = core;

	QVector<Module *> arr;

	size_t id = 0;

	for (std::vector<ILexer *>::iterator it = getLexers().begin(); it != getLexers().end(); ++it)
	{
		arr << new Module("LexerModule", 1, (*it)->getName(), 1, id++);
	}

	return arr;
}

QObject* Lexers_Library::CreateModuleInstance( int id, QObject *parent )
{
	if (id < getLexers().size())
	{
		return dynamic_cast<QObject*>(getLexers()[id]);
	}
	return NULL;
}

QT_BEGIN_NAMESPACE
Q_EXPORT_PLUGIN2(Lib_Lexers, Lexers_Library)
QT_END_NAMESPACE