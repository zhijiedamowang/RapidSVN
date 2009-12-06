/*
 * ====================================================================
 * Copyright (c) 2002-2009 The RapidSvn Group.  All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the file GPL.txt.  
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://rapidsvn.tigris.org/.
 * ====================================================================
 */

// wxWidgets
#include "wx/wx.h"
#include "wx/grid.h"
#include "wx/listctrl.h"
#include "wx/valgen.h"

// stl
#include "svncpp/vector_wrapper.hpp"

// svncpp
#include "svncpp/exception.hpp"

// app
#include "listed_dlg.hpp"
#include "utils.hpp"

enum
{
  ID_New = 0x2000,
  ID_Edit,
  ID_Delete,
  ID_List,
  ID_Name,
  ID_Value
};

enum
{
  EDIT_NEW,
  EDIT_EDIT
};


class ListCtrl : public wxListView
{
public:
  ListCtrl(wxWindow * parent)
      : wxListView(parent, ID_List, wxDefaultPosition,
                   wxSize(350, 150), wxLC_REPORT |
                   wxLC_SINGLE_SEL)
  {
    wxListItem info;

    info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_WIDTH;
    info.m_col = 0;
    info.m_width = 120;
    info.m_text = _("Name");
    InsertColumn(0, info);

    info.m_col = 1;
    info.m_width = 240;
    info.m_text = _("Value");
    InsertColumn(1, info);
  }

  /**
   * Adds/modifys an entry in the list.
   * If the entry is not already there (identified by name),
   * add it to the list, otherwise modify the value.
   *
   * @param name
   * @param value
   * @return id of new item if inserted or old item if updated
   */
  long
  SetEntry(const wxString & name, const wxString & value)
  {
    long id = FindItem(-1, name, false);

    if (-1 != id)
      m_values [id] = value;
    else
    {
      id = GetItemCount();
      InsertItem(id, name);
      m_values.push_back(value);
    }

    wxListItem item;
    item.m_itemId = id;
    item.m_text = value;
    item.m_col = 1;
    item.m_mask = wxLIST_MASK_TEXT;
    SetItem(item);

    return id;
  }

  /**
   * returns the name/value pair for the selected item in the
   * list
   *
   * @param name
   * @param value
   * @retval false nothing selected
   */
  bool
  GetSelectedEntry(wxString & name, wxString & value)
  {
    long id = GetFirstSelected();
    if (id == -1)
      return false;

    GetEntryAtIndex(id, name, value);
    return true;
  }

  /**
   * returns the value of the entry at the given position
   *
   * @param id zero based position int the list
   * @param name property name
   * @param value property value
   */
  void
  GetEntryAtIndex(long id, wxString & name, wxString & value)
  {
    // get name
    name = GetItemText(id);

    // get value
    value = m_values[id];
  }

private:
  std::vector<wxString> m_values;
};

/**
 * private class for the dialog.
 * this edits one single name/value pair
 */
class EntryDlg : public wxDialog
{
public:
  /**
   * Constructor
   */
  EntryDlg(wxWindow * parent, const wxString & title)
      : wxDialog(parent, -1, title, wxDefaultPosition),
      m_readOnly(false)
  {
    wxStaticText * labelName = new wxStaticText(this, -1, _("Name"));
    wxStaticText * labelValue = new wxStaticText(this, -1, _("Value"));
    wxTextCtrl * textName = new wxTextCtrl(this, ID_Name);

    wxTextCtrl * textValue =
      new wxTextCtrl(this, ID_Value, wxEmptyString, wxDefaultPosition,
                     wxSize(300, 100), wxTE_MULTILINE);

    wxFlexGridSizer * textSizer = new wxFlexGridSizer(2, 5, 5);
    textSizer->Add(labelName);
    textSizer->Add(textName, 1, wxEXPAND);
    textSizer->Add(labelValue);
    textSizer->Add(textValue, 1, wxEXPAND);

    wxButton * okButton = new wxButton(this, wxID_OK, _("OK"));
    wxButton * cancelButton = new wxButton(this, wxID_CANCEL, _("Cancel"));
    wxBoxSizer * buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);

    wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->Add(textSizer, 1, wxALL | wxEXPAND, 5);
    mainSizer->Add(buttonSizer, 0, wxALL | wxALIGN_RIGHT , 5);

    okButton->SetDefault();

    SetAutoLayout(true);
    SetSizer(mainSizer);

    mainSizer->SetSizeHints(this);
    mainSizer->Fit(this);

    m_buttonOk = okButton;
    m_textName = textName;
    m_textValue = textValue;
    m_labelName = labelName;
    m_labelValue = labelValue;

    m_mode = EDIT_NEW;
  }

  /**
   * Execute the dialog
   *
   * @param caption caption of the dialog
   * @param value name of the property
   * @retval true if OK button was clicked
   */
  bool
  Execute(const int mode,
          wxString & name,
          wxString & value)
  {
    switch (mode)
    {
    case EDIT_NEW:
    case EDIT_EDIT:
      break;
    default:
      // invalid mode, exit
      return false;
    }
    m_mode = mode;

    SetReturnCode(wxID_CANCEL);
    m_textName->Enable(mode == EDIT_NEW);
    TrimString(name);
    TrimString(value);
    m_textName->SetValue(name);
    m_textValue->SetValue(value);

    CheckButtons();

    bool ok = ShowModal() == wxID_OK;

    if (ok)
    {
      name = m_textName->GetValue();
      value = m_textValue->GetValue();
      TrimString(name);
      TrimString(value);
    }

    return ok;
  }

  void SetNameCaption(const wxString & caption)
  {
    m_labelName->SetLabel(caption);
  }

  void SetValueCaption(const wxString & caption)
  {
    m_labelValue->SetLabel(caption);
  }

  void SetReadOnly(bool value)
  {
    m_readOnly = value;
    m_textName->Enable((m_mode == EDIT_NEW) && !value);
    m_textValue->Enable(!value);
    CheckButtons();
  }

private:
  bool m_readOnly;
  wxTextCtrl * m_textName;
  wxTextCtrl * m_textValue;
  wxStaticText * m_labelName;
  wxStaticText * m_labelValue;
  wxButton * m_buttonOk;
  int m_mode;

  void
  OnName(wxCommandEvent &)
  {
    CheckButtons();
  }

  void
  CheckButtons()
  {
    wxString name = m_textName->GetValue();
    TrimString(name);
    m_buttonOk->Enable((name.Length() > 0) && !m_readOnly);
  }

private:
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(EntryDlg, wxDialog)
  EVT_TEXT(ID_Name, EntryDlg::OnName)
END_EVENT_TABLE()

struct ListEditorDlg::Data
{
  wxWindow * wnd;
  wxString nameCaption;
  wxString valueCaption;
  bool readOnly;
  ListCtrl * listCtrl;
  wxString addTitle;
  wxString editTitle;

public:
  Data(wxWindow * wnd_)
    : wnd(wnd_), nameCaption(_("Name")), valueCaption(_("Value")),
      readOnly(false), listCtrl(0), addTitle(_("Add")),
      editTitle(_("Edit"))
  {
  }


  long
  GetSelection() const
  {
    if (!listCtrl)
      return 0;

    return listCtrl->GetFirstSelected();
  }


  /**
   * shows the dialog for a property to edit or add.
   *
   * @param edit EDIT_NEW or EDIT_EDIT
   */
  void
  Edit(int mode)
  {
    wxString name, value, title;

    if (mode != EDIT_EDIT)
      title = addTitle;
    else
    {
      title = editTitle;
      listCtrl->GetSelectedEntry(name, value);
    }

    EntryDlg dlg(wnd, title);
    dlg.SetReadOnly(readOnly);
    if (!dlg.Execute(mode, name, value))
      return;

    listCtrl->SetEntry(name, value);
  }

};


ListEditorDlg::ListEditorDlg(wxWindow * parent,
                             const wxString & title)
    : ListEditorDlgBase(parent, -1, title)
{
  m = new Data(this);
  m->listCtrl = new ListCtrl(this);

  m_listSizer->Add(m->listCtrl, 1, wxALL | wxEXPAND, 2);

  m_mainSizer->SetSizeHints(this);
  m_mainSizer->Fit(this);

  Layout();
  CentreOnParent();

  CheckControls();

  m->listCtrl->Connect(wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler(ListEditorDlg::OnSelected), NULL, this );
}

ListEditorDlg::~ListEditorDlg()
{
  m->listCtrl->Disconnect(wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler(ListEditorDlg::OnSelected), NULL, this );

  delete m;
}

void
ListEditorDlg::OnNew(wxCommandEvent & WXUNUSED(event))
{
  m->Edit(EDIT_NEW);
}

void
ListEditorDlg::OnEdit(wxCommandEvent & WXUNUSED(event))
{
  m->Edit(EDIT_EDIT);
}

void
ListEditorDlg::OnDelete(wxCommandEvent & WXUNUSED(event))
{
  long id = m->GetSelection();

  if (-1 == id)
    return;

  m->listCtrl->DeleteItem(id);

  CheckControls();
}

void
ListEditorDlg::OnSelected(wxListEvent & WXUNUSED(event))
{
  CheckControls();
}

void
ListEditorDlg::SetCaption(const wxString & caption)
{
  m_listSizer->GetStaticBox()->SetLabel(caption);
}

void
ListEditorDlg::SetNameCaption(const wxString & caption)
{
  m->nameCaption = caption;
}

void
ListEditorDlg::SetValueCaption(const wxString & caption)
{
  m->valueCaption = caption;
}

void
ListEditorDlg::SetAddTitle(const wxString & title)
{
  m->addTitle = title;
}

void
ListEditorDlg::SetEditTitle(const wxString & title)
{
  m->editTitle = title;
}

void
ListEditorDlg::DeleteAllEntries()
{
  m->listCtrl->DeleteAllItems();
}

long
ListEditorDlg::SetEntry(const wxString & name, const wxString & value)
{
  return m->listCtrl->SetEntry(name, value);
}

void
ListEditorDlg::GetEntryAtIndex(long id, wxString & name, wxString & value) const
{
  m->listCtrl->GetEntryAtIndex(id, name, value);
}

long
ListEditorDlg::GetEntryCount() const
{
  return m->listCtrl->GetItemCount();
}

long
ListEditorDlg::FindEntry(const wxString & name) const
{
  return m->listCtrl->FindItem(-1, name, false);
}

void
ListEditorDlg::SetReadOnly(bool value)
{
  m->readOnly = value;

  m_buttonEdit->SetLabel(value ? _("View...") : _("Edit..."));

  CheckControls();
}

void
ListEditorDlg::CheckControls()
{
  bool isSelected = m->GetSelection() >= 0;

  EnableCtrl(m_buttonNew, !m->readOnly);
  EnableCtrl(m_buttonOK, !m->readOnly);
  EnableCtrl(m_buttonEdit, !m->readOnly && isSelected);
  EnableCtrl(m_buttonDelete, !m->readOnly && isSelected);
}


/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rapidsvn-dev.el")
 * end:
 */
