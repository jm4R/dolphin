/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>

#include <qclipboard.h>
#include <qpopupmenu.h>
#include <qpainter.h>

#include <klocale.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <kapplication.h>

#include <kaction.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kmessagebox.h>
#include <klineeditdlg.h>
#include <krun.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>

#include "kebbookmarkexporter.h"

// DESIGN - shuffle, sort out includes in general

#include "toplevel.h"
#include "commands.h"
#include "importers.h"
#include "favicons.h"
#include "testlink.h"
#include "search.h"
#include "listview.h"
#include "mymanager.h"

#include "actionsimpl.h"

void ActionsImpl::slotExpandAll() {
   KEBTopLevel::self()->setAllOpen(true);
}

void ActionsImpl::slotCollapseAll() {
   KEBTopLevel::self()->setAllOpen(false);
}

ActionsImpl* ActionsImpl::s_self = 0;

/* ------------------------------------------------------------- */
//                             CLIPBOARD
/* ------------------------------------------------------------- */

void ActionsImpl::slotCut() {
   slotCopy();
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Cut Items"), listview->selectedItems());
   KEBTopLevel::self()->didCommand(mcmd);
}

void ActionsImpl::slotCopy() {
   // this is not a command, because it can't be undone
   Q_ASSERT(listview->selectedItems()->count() != 0);
   QValueList<KBookmark> bookmarks = listview->itemsToBookmarks(listview->selectedItems());
   KBookmarkDrag* data = KBookmarkDrag::newDrag(bookmarks, 0 /* not this ! */);
   kapp->clipboard()->setData(data, QClipboard::Clipboard);
}

void ActionsImpl::slotPaste() {
   KMacroCommand *mcmd = 
      CmdGen::self()->insertMimeSource(
           i18n("Paste"), 
           kapp->clipboard()->data(QClipboard::Clipboard), 
           listview->userAddress());
   KEBTopLevel::self()->didCommand(mcmd);
}

/* ------------------------------------------------------------- */
//                             POS_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotNewFolder() {
   // TODO - move this into a sanely named gui class in kio/bookmarks?
   KLineEditDlg dlg(i18n("New folder:"), "", 0);
   dlg.setCaption(i18n("Create New Bookmark Folder"));
   // text is empty by default, therefore disable ok button.
   dlg.enableButtonOK(false);
   if (!dlg.exec()) {
      return;
   }
   CreateCommand *cmd = new CreateCommand(
                              listview->userAddress(),
                              dlg.text(), "bookmark_folder", /*open*/ true);
   KEBTopLevel::self()->addCommand(cmd);
}

void ActionsImpl::slotNewBookmark() {
   CreateCommand * cmd = new CreateCommand(
                               listview->userAddress(),
                               QString::null, QString::null, KURL());
   KEBTopLevel::self()->addCommand(cmd);
}

void ActionsImpl::slotInsertSeparator() {
   CreateCommand * cmd = new CreateCommand(listview->userAddress());
   KEBTopLevel::self()->addCommand(cmd);
}

void ActionsImpl::slotImport() { 
   ImportCommand* import = ImportCommand::performImport(sender()->name()+6, KEBTopLevel::self());
   if (!import) {
      return;
   }
   KEBTopLevel::self()->addCommand(import);
   KEBListViewItem *item = listview->getItemAtAddress(import->groupAddress());
   if (item) {
      listview->setCurrent(item);
   }
}

void ActionsImpl::slotExportNS() {
   MyManager::self()->doExport(false);
}

void ActionsImpl::slotExportMoz() {
   MyManager::self()->doExport(true);
}

/* ------------------------------------------------------------- */
//                            NULL_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotShowNS() {
   bool shown = KEBTopLevel::self()->nsShown();
   BkManagerAccessor::mgr()->setShowNSBookmarks(shown);
   KEBTopLevel::self()->setModifiedFlag(true);
}

void ActionsImpl::slotCancelFavIconUpdates() {
   FavIconsItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotCancelAllTests() {
   TestLinkItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotCancelSearch() {
   SearchItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotTestAll() {
   TestLinkItrHolder::self()->insertItr(new TestLinkItr(listview->allBookmarks()));
}

void ActionsImpl::slotUpdateAllFavIcons() {
   FavIconsItrHolder::self()->insertItr(new FavIconsItr(listview->allBookmarks()));
}

/* ------------------------------------------------------------- */
//                             ITR_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotTestSelection() {
   TestLinkItrHolder::self()->insertItr(new TestLinkItr(listview->selectedBookmarksExpanded()));
}

void ActionsImpl::slotUpdateFavIcon() {
   FavIconsItrHolder::self()->insertItr(new FavIconsItr(listview->selectedBookmarksExpanded()));
}

void ActionsImpl::slotSearch() {
   // TODO
   // also, need to think about limiting size of itr list to <= 1
   // or, generically. itr's shouldn't overlap. difficult problem...
   bool ok;
   QString text = KLineEditDlg::getText("Find string in bookmarks:", "", &ok, KEBTopLevel::self());
   SearchItr* itr = new SearchItr(listview->allBookmarks());
   itr->setText(text);
   SearchItrHolder::self()->insertItr(itr);
}

/* ------------------------------------------------------------- */
//                            GROUP_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotSort() {
   KBookmark bk = listview->selectedBookmark();
   Q_ASSERT(bk.isGroup());
   SortCommand *cmd = new SortCommand(i18n("Sort Alphabetically"), bk.address());
   KEBTopLevel::self()->addCommand(cmd);
}

/* ------------------------------------------------------------- */
//                           SELC_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotDelete() {
   KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Delete Items"), listview->selectedItems());
   KEBTopLevel::self()->didCommand(mcmd);
}

void ActionsImpl::slotOpenLink() {
   QValueList<KBookmark> bks = listview->itemsToBookmarks(listview->selectedItems());
   QValueListIterator<KBookmark> it;
   for (it = bks.begin(); it != bks.end(); ++it) {
      if ((*it).isGroup() || (*it).isSeparator()) {
         continue;
      }
      (void)new KRun((*it).url());
   }
}

/* ------------------------------------------------------------- */
//                          ITEM_ACTION
/* ------------------------------------------------------------- */

void ActionsImpl::slotRename() {
   listview->rename(KEBListView::NameColumn);
}

void ActionsImpl::slotChangeURL() {
   listview->rename(KEBListView::UrlColumn);
}

void ActionsImpl::slotSetAsToolbar() {
   KBookmark bk = listview->selectedBookmark();
   Q_ASSERT(bk.isGroup());
   KMacroCommand *mcmd = CmdGen::self()->setAsToolbar(bk);
   KEBTopLevel::self()->addCommand(mcmd);
}

void ActionsImpl::slotChangeIcon() {
   KBookmark bk = listview->selectedBookmark();
   KIconDialog dlg(KEBTopLevel::self());
   QString newIcon = dlg.selectIcon(KIcon::Small, KIcon::FileSystem);
   if (newIcon.isEmpty()) {
      return;
   }
   EditCommand *cmd = new EditCommand(
                            bk.address(),
                            EditCommand::Edition("icon", newIcon),
                            i18n("Icon"));
   KEBTopLevel::self()->addCommand(cmd);
}

#include "actionsimpl.moc"
