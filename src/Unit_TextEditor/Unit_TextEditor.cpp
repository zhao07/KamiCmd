#include "Unit_TextEditor/Unit_TextEditor.h"
#include "Unit_TextEditor/library.h"
#include "UnitManager_Tabs/ActionManager.h"
#include <QRegExp>
#include <vector>
#include "../UnitManager_Tabs/ActionManager.h"
#include "Unit_TextEditor/SciSettings.h"
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexer.h>

#include "ILexerPlugin.h"

std::map<QString, QsciLexer*> & lexers()
{
	static std::map<QString, QsciLexer*> lexers_;
	return lexers_;
}

std::vector<std::pair<QRegExp, QString> > & assoc()
{
	static std::vector<std::pair<QRegExp, QString> > assoc_;
	return assoc_;
}

QsciLexer * getLexer(QString const & name)
{
	if (lexers().count(name))
	{
		return lexers()[name];
	}
	
	return 0;
}

QString const & extToLexer(QString const & name)
{
	static QString empty;

	for (std::vector<std::pair<QRegExp, QString> >::iterator it = 
		assoc().begin(); it != assoc().end(); ++it)
	{
		if (it->first.exactMatch(name))
		{
			return it->second;
		}
	}
	
	return empty;
}

bool & lexersLoaded()
{
	static bool loaded = false;
	return loaded;
}

void Unit_TextEditor::loadModules()
{
	if (lexersLoaded())
		return;

	QVector<const Module *> lexerUnits = g_Core->GetModulesInfo("LexerModule", 1);
		
	foreach(const Module * lexerUnit, lexerUnits)
	{
		ILexer * lexer = dynamic_cast<ILexer*>(g_Core->QueryModule("LexerModule", 1, lexerUnit->name));
		if (lexer)
		{
			lexers()[lexer->getName()] = lexer->getLexer();
			foreach(QString const & wildcard, lexer->getMask().ext)
			{
				assoc().push_back(std::make_pair(QRegExp(wildcard, Qt::CaseInsensitive, QRegExp::Wildcard), lexer->getName()));
			}
		}
	}

	lexersLoaded() = true;
}

Unit_TextEditor::Unit_TextEditor(QWidget *parent, SciSettings * settings, ICoreFunctions * core)
	: IUnit(parent), settings_(settings), g_Core(core)
{
	setCentralWidget(editor = new QsciScintilla(this));	
	connect(editor, SIGNAL(textChanged()), this, SLOT(onEdit()));
	loadModules();

	ld = new LexersDialog(this);

	connect(ld, SIGNAL(setLexer(QsciLexer*)), this, SLOT(setLexer(QsciLexer*)));

	if (!(am = dynamic_cast<ActionManager *>(g_Core->QueryModule("ActionManager", 1))))
	{
		g_Core->DebugWrite("TextEditor", "ActionManager module not found", ICoreFunctions::Error);
		QMessageBox::warning(this, "TextEditor", "ActionManager module not found");
	}

	Actions << new QAction(QIcon(), tr("Select Lexer"), this);
	Actions.last()->setShortcut(Qt::CTRL + Qt::Key_L);
	Actions.last()->setShortcutContext(Qt::WindowShortcut);
	Actions.last()->setData("TextEditor");
	connect(Actions.last(), SIGNAL(triggered()), this, SLOT(selectLexer()));
	Actions << new QAction(QIcon(), tr("Editor settings"), this);
	Actions.last()->setShortcut(Qt::CTRL + Qt::Key_E);
	Actions.last()->setShortcutContext(Qt::WindowShortcut);
	Actions.last()->setData("TextEditor");
	connect(Actions.last(), SIGNAL(triggered()), settings, SLOT(showSettings()));
	am->RegisterActions(Actions);
	addActions(Actions);

	connect(settings_, SIGNAL(settingsChanged()), this, SLOT(updateSettings()));
	settings_->applySettings(editor);
}

QString Unit_TextEditor::GetText()
{
	return text;
}

void Unit_TextEditor::SaveState(QSettings & set)
{

}

void Unit_TextEditor::LoadState(QSettings & set)
{
}

void Unit_TextEditor::Create( IUnit *createdFrom )
{
	if (hostPanel = dynamic_cast<IPanel *>(createdFrom))
	{
		const FileInfo *const info = hostPanel->GetCurrentFile();
	
		QFile file(info->path + info->name);

		if (!file.open(QFile::ReadOnly)) {
			QMessageBox::warning(this, tr("Application"),
								 tr("Cannot read file %1:\n%2.")
								 .arg(info->path + info->name)
								 .arg(file.errorString()));
			return;
		}

		QTextStream in(&file);
		QApplication::setOverrideCursor(Qt::WaitCursor);

		editor->setText(in.readAll());

		text = info->name;
		path = info->path;
		edited = false;
		//emit TextChanged();

		QFileInfo qinfo(info->path + info->name);

		QString lex = extToLexer(info->name);

		if (lex != "")
		{
			editor->setLexer(getLexer(lex));
		}
		
		QApplication::restoreOverrideCursor();
	}
}

QIcon Unit_TextEditor::GetIcon()
{
	return QIcon();
}

void Unit_TextEditor::onEdit()
{
	if (edited)
		return;

	edited = true;
	text = text + "*";
	//emit TextChanged();
}

void Unit_TextEditor::selectLexer()
{
	ld->show(lexers(), editor->lexer());
}

void Unit_TextEditor::setLexer(QsciLexer * l)
{
	editor->setLexer(l);
}

void Unit_TextEditor::updateSettings()
{
	settings_->applySettings(editor);
}

Unit_TextEditor::~Unit_TextEditor()
{
	am->UnregisterActions(Actions);
}