/***************************************************************************
 *   Copyright Simon Persson                                               *
 *   simonpersson1@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "filedigger.h"
#include "mergedvfs.h"

#include <git2/threads.h>

#include <KAboutData>
#include <KLocalizedString>
#include <KApplication>
#include <KCmdLineArgs>
#include <KMessageBox>

static const char version[] = "0.5.1";
static const char description[] = I18N_NOOP("Browser for bup archives.");
#include <QFile>
#include <QTextStream>

int main(int pArgCount, char **pArgArray) {
	KAboutData lAbout("kup-filedigger", "kup", ki18n("File Digger"), version, ki18n(description),
	                  KAboutData::License_GPL, ki18n("Copyright (C) 2013 Simon Persson"),
	                  KLocalizedString(), 0, "simonpersson1@gmail.com");
	lAbout.addAuthor(ki18n("Simon Persson"), KLocalizedString(), "simonpersson1@gmail.com");
	lAbout.setTranslator(ki18nc("NAME OF TRANSLATORS", "Your names"), ki18nc("EMAIL OF TRANSLATORS", "Your emails"));
	KCmdLineArgs::init(pArgCount, pArgArray, &lAbout);

	KCmdLineOptions lOptions;
	lOptions.add("b").add("branch <branch name>", ki18n("Name of the branch to be opened."), "kup");
	lOptions.add("+<repository path>", ki18n("Path to the bup repository to be opened."));
	KCmdLineArgs::addCmdLineOptions(lOptions);

	KApplication lApp;
	KCmdLineArgs *lParsedArguments = KCmdLineArgs::parsedArgs();
	if(lParsedArguments->count() != 1) {
		KCmdLineArgs::usageError(ki18nc("Error message at startup",
		                                "You must supply the path to a bup or git repository that "
		                                "you wish to open for viewing.").toString());
	}

	// This needs to be called first thing, before any other calls to libgit2.
	git_threads_init();
	MergedRepository *lRepository = new MergedRepository(NULL, lParsedArguments->arg(0),
	                                                     lParsedArguments->getOption("branch"));
	if(!lRepository->open()) {
		KMessageBox::sorry(NULL, i18nc("@info:label messagebox, %1 is a folder path",
		                               "The backup archive \"%1\" could not be opened. Check if the backups really are located there.",
		                               lParsedArguments->arg(0)));
		return 1;
	}
	if(!lRepository->readBranch()) {
		if(!lRepository->permissionsOk()) {
			KMessageBox::sorry(NULL, i18nc("@info:label messagebox",
			                               "You do not have permission needed to read this backup archive."));
			return 2;
		} else {
			lRepository->askForIntegrityCheck();
			return 3;
		}
	}

	FileDigger *lFileDigger = new FileDigger(lRepository);
	lFileDigger->show();
	int lRetVal = lApp.exec();
	delete lRepository;
	git_threads_shutdown();
	return lRetVal;
}
