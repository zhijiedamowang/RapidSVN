
#ifndef _PROPERTY_DLG_H_INCLUDED_
#define _PROPERTY_DLG_H_INCLUDED_

#include "svncpp/property.h"
#include <wx/grid.h>

class PropertyGrid : public wxGrid
{
public:
  PropertyGrid (wxWindow * parent, svn::Property * property);
  void FillList ();

private:
  svn::Property * m_property;
  void InitializeData ();
  
  DECLARE_EVENT_TABLE ()
};

class PropertyDlg : public wxDialog
{
public:
  PropertyDlg (wxWindow * parent, svn::Property * property);
private:
  svn::Property * m_property;
  PropertyGrid * propGrid;

  void InitializeData ();

  /**
   * Save all of the valid property key/value pairs.
   */
  void SaveData ();

  /**
   * Remove all of the property key/value pairs that do not 
   * exist any more.
   */
  void RemoveData ();

  void OnClose (wxCommandEvent & event);
  void OnOK (wxCommandEvent & event);
  
  DECLARE_EVENT_TABLE ()
};

#endif
