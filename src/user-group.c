/*  group-service 
*   Copyright (C) 2018  zhuyaliang https://github.com/zhuyaliang/
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "user-group.h"
#include "user-info.h"
#include "user-share.h"
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>

enum
{
    PROP_0,
    PROP_GID,
    PROP_GROUP_NAME,
    PROP_LOCAL_GROUP,
};
G_DEFINE_TYPE (UserGroup, user_group, G_TYPE_OBJECT)

static void group_finalize (GObject *object)
{
    UserGroup *group;

    group = USERGROUP (object);
    g_free (group->GroupName);
}

static void user_group_class_init (UserGroupClass *class)
{
	GObjectClass *gobject_class;
    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = group_finalize;

}

static void user_group_init (UserGroup *group)
{
    group->GroupName = NULL;
    group->GroupId   = -1;
}

UserGroup * group_new (const char *GroupName,gid_t gid)
{
    UserGroup *group;

    group            = g_object_new (USER_TYPE_GROUP, NULL);
	group->GroupName = g_strdup(GroupName);
	group->GroupId   = gid;
    return group;
}
static UserGroup * GroupInit (GasGroup *gas)
{
    const char *name = NULL;
    gid_t gid;
    UserGroup *usergroup;

    name = gas_group_get_group_name(gas);
    gid  = gas_group_get_gid(gas);
    if(name == NULL)
    {
        g_error("Failed to get group name !!!\r\n");
        return NULL;
    }
    usergroup            = group_new(name,gid);
    usergroup->gas       = gas;
    usergroup->GroupName = strdup(name);

    return usergroup;
}
enum
{
  COLUMN_FIXED,
  COLUMN_GROUPNAME,
  COLUMN_GROUPID,
  NUM_COLUMNS
};
enum
{
  COLUMN_SELECT,
  COLUMN_USERNAME,
  COLUMN_USERID,
  NUM_USER
};
enum
{
  COLUMN_ICON,
  COLUMN_GROUPN,
  COLUMN_GROUPI,
  NUM_L
};
static void addlistdata(GtkListStore *store,
                        UserGroup    *group,
                        const gchar  *name);
static gboolean CheckGroupNameUsed (const gchar *name)
{
    struct group *grent;
    grent = getgrnam (name);

    return grent != NULL;
}    
static gboolean CheckGroupName (const gchar *name, gchar **Message)
{
    gboolean empty;
    gboolean in_use;
    gboolean valid;
    const gchar *c;

    if (name == NULL || name[0] == '\0') 
    {
        empty = TRUE;
        in_use = FALSE;
    } 
    else 
    {
        empty = FALSE;
        in_use = CheckGroupNameUsed (name);
    }
    valid = TRUE;
    if (!in_use && !empty) 
    {
        for (c = name; *c; c++) 
        {
            if (! ((*c >= 'a' && *c <= 'z') ||
                   (*c >= 'A' && *c <= 'Z') ||
                   (*c >= '0' && *c <= '9') ||
                   (*c == '_') || (*c == '.') ||
                   (*c == '-' && c != name)))
               valid = FALSE;
        }
    }

    valid = !empty && !in_use && valid;
    if (!empty && (in_use || !valid))
    {
        if (in_use) 
        {
            *Message = g_strdup (_("Repeat of group name.Please try another"));
        }
        else if (name[0] == '-') 
        {
            *Message = g_strdup (_("The groupname cannot start with a - ."));
        }
        else 
        {
            *Message = g_strdup (_("The groupname should only consist of upper and lower case \nletters from a-z,digits and the following characters: . - _"));
        }
    }

    return valid;
}

            
static gboolean QuitGroupWindow (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer   data)
{
    GroupsManage *gm = (GroupsManage *)data;
    g_free(gm->username);
    g_slist_free_full (gm->GroupsList,g_object_unref);
    if(gm->NewGroupUsers != NULL)
        g_slist_free(gm->NewGroupUsers);   
    gtk_widget_destroy(gm->GroupsWindow);
	gtk_widget_show(WindowLogin);
    return TRUE;
}    

static void CloseGroupWindow (GtkWidget *widget, gpointer data)
{
    GroupsManage *gm = (GroupsManage *)data;
    g_free(gm->username);
    g_slist_free_full (gm->GroupsList,g_object_unref);
    if(gm->NewGroupUsers != NULL)
        g_slist_free(gm->NewGroupUsers);   
    gtk_widget_destroy(gm->GroupsWindow);
	gtk_widget_show(WindowLogin);
}    
static void AddUserToGroup(GSList *list,GasGroup *group)
{
	GSList *node;
	const char *name;

	if(g_slist_length(list) <= 0)
	{
		return;
	}		

	for(node = list; node; node = node->next)
	{
		name = node->data;
		gas_group_add_user_group(group,name);	
	}		
}		
static void CreateNewGroup(GtkWidget *widget, gpointer data)
{
    GroupsManage *gm = (GroupsManage *)data;
    gboolean      Valid;
    char         *Message = NULL;
    const char   *s;
	GasGroup     *gas;
	UserGroup    *group;
	GError       *error = NULL;
	int           ret;	
	GasGroupManager *manage;
    
    s = gtk_entry_get_text(GTK_ENTRY(gm->EntryGroupName));
    Valid = CheckGroupName(s,&Message);  
    if(Valid == FALSE)
    {
        MessageReport(_("Create New Group"),Message,ERROR);
        return;
    }
	manage = gas_group_manager_get_default();
	gas    = gas_group_manager_create_group(manage,s,&error);
	if(gas == NULL)
	{
		MessageReport(_("Create New Group Faild"),error->message,ERROR);
		if(error != NULL)
		{
			g_error_free(error);
			return;
		}			
	}
	AddUserToGroup(gm->NewGroupUsers,gas);
	group = GroupInit(gas);
	gm->GroupsList = g_slist_append(gm->GroupsList,g_object_ref(group));
	ret = MessageReport(_("Create User Group"),_("Create User Group Successfully,Do you want to continue creating"),QUESTION);
	if(ret == GTK_RESPONSE_NO)
	{
		CloseGroupWindow(NULL,gm);
	}		
	addlistdata (gm->SwitchStore,group,gm->username);
}    
static int GetGroupNum(GSList *List)
{
    return g_slist_length(List);
}    
static gboolean GetUserIsGroup(GasGroup *gas,const gchar *user)
{
    return gas_group_user_is_group(gas,user);
}

static const gchar *GetGroupName(GasGroup *gas)
{
    return gas_group_get_group_name(gas);
}    

static int GetGroupGid(GasGroup *gas)
{
    return gas_group_get_gid(gas);
}    
static UserGroup *GetGroupObject (GSList *List,guint gid)
{
    GSList *node;
    guint groupgid;

    for (node = List; node != NULL; node = node->next) 
    {
        UserGroup *group = (UserGroup*)node->data;
        groupgid = GetGroupGid (group->gas);
        if (groupgid == gid) 
        {
            return group;
        }
    }    
        return NULL;
}    

static void UserSelectGroup (GtkCellRendererToggle *cell,
                             gchar                 *path_str,
                             gpointer               data)
{
    GroupsManage *gm       = (GroupsManage *)data;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeSwitch);
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkTreePath  *path;
    gboolean      fixed;
    uint          gid;  
    UserGroup     *group; 

    model = gtk_tree_view_get_model (treeview);
    path  = gtk_tree_path_new_from_string (path_str);
    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COLUMN_FIXED, &fixed, -1);
    gtk_tree_model_get (model, &iter, COLUMN_GROUPID, &gid, -1); 
    
    group = GetGroupObject(gm->GroupsList,gid);
    if(fixed == TRUE)
    {
        gas_group_remove_user_group(group->gas,gm->username);
    }   
    else
    {
        gas_group_add_user_group(group->gas,gm->username);
    }    
    /* do something with the value */
    fixed ^= 1;
    /* set new value */
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_FIXED, fixed, -1);

    /* clean up */
    gtk_tree_path_free (path);
}
static void NewGroupSelectUsers (GtkCellRendererToggle *cell,
                                 gchar                 *path_str,
                                 gpointer               data)
{
    GroupsManage *gm       = (GroupsManage *)data;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeCreate);
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkTreePath  *path;
    gboolean      fixed;
    const gchar  *name;  

    model = gtk_tree_view_get_model (treeview);
    path  = gtk_tree_path_new_from_string (path_str);
    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, COLUMN_SELECT,   &fixed, -1);
    gtk_tree_model_get (model, &iter, COLUMN_USERNAME, &name, -1); 
    if(fixed == TRUE)
    {
        gm->NewGroupUsers = g_slist_append(gm->NewGroupUsers,name);
    }   
    else
    {
        gm->NewGroupUsers = g_slist_remove(gm->NewGroupUsers,name);
    }    
    /* do something with the value */
    fixed ^= 1;
    /* set new value */
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_SELECT, fixed, -1);

    /* clean up */
    gtk_tree_path_free (path);
}
static void addlistdata(GtkListStore *store,
				        UserGroup    *group,
						const gchar  *name)
{
    GtkTreeIter   iter;
	gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        COLUMN_FIXED,     GetUserIsGroup(group->gas,name),
                        COLUMN_GROUPNAME, GetGroupName(group->gas),
                        COLUMN_GROUPID,   GetGroupGid(group->gas),
                        -1);

}		
static GtkTreeModel * CreateSwicthModel (GSList *List,const gchar *name)
{
 	GtkListStore *SwitchStore = NULL;
    gint          i = 0;
    int           GroupNum = 0;
    UserGroup    *group;

    GroupNum = GetGroupNum(List);
    SwitchStore = gtk_list_store_new (NUM_COLUMNS,
                                      G_TYPE_BOOLEAN,
                                      G_TYPE_STRING,
                                      G_TYPE_UINT);

    
    for (i = 0; i < GroupNum ; i++)
    {
        group = g_slist_nth_data(List,i); 
        if(group == NULL)
        {
            g_error("No such the Group!!!\r\n");
            break;
        }    
		addlistdata(SwitchStore,group,name);
    }
    return GTK_TREE_MODEL (SwitchStore);
}

static GtkTreeModel * CreateAddUsersModel (GSList *List)
{
    gint          i = 0;
    GtkListStore *store;
    GtkTreeIter   iter;
    int           UserNum = 0;
    UserInfo     *user;

    UserNum = GetGroupNum(List);
    store = gtk_list_store_new (NUM_USER,
                                G_TYPE_BOOLEAN,
                                G_TYPE_STRING,
                                G_TYPE_UINT);

    
    for (i = 0; i < UserNum ; i++)
    {
        user = g_slist_nth_data(List,i); 
        if(user == NULL)
        {
            g_error("No such the User!!!\r\n");
            break;
        }    
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            COLUMN_SELECT,   FALSE,
                            COLUMN_USERNAME, GetUserName(user->ActUser),
                            COLUMN_USERID,   GetUserUid (user->ActUser),
                            -1);
    }

    return GTK_TREE_MODEL (store);
}
static void AddSwitchGroupColumns (GroupsManage *gm)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeSwitch);
    
    renderer = gtk_cell_renderer_toggle_new (); 
    g_signal_connect (renderer, 
                     "toggled",
                      G_CALLBACK (UserSelectGroup), 
                      gm);

    column = gtk_tree_view_column_new_with_attributes ("Select",
                                                        renderer,
                                                       "active", COLUMN_FIXED,
                                                        NULL);

    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                     GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new (); 
    column = gtk_tree_view_column_new_with_attributes ("Group Name",
                                                        renderer,
                                                       "text",
                                                        COLUMN_GROUPNAME,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_GROUPNAME);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Group ID",
                                                        renderer,
                                                       "text",
                                                        COLUMN_GROUPID,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_GROUPID);
    gtk_tree_view_append_column (treeview, column);

}
static void AddSelectUsersColumns (GroupsManage *gm)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeView  *treeview = GTK_TREE_VIEW(gm->TreeCreate);
    
    renderer = gtk_cell_renderer_toggle_new (); 
    g_signal_connect (renderer, 
                     "toggled",
                      G_CALLBACK (NewGroupSelectUsers), 
                      gm);

    column = gtk_tree_view_column_new_with_attributes ("Select",
                                                        renderer,
                                                       "active", COLUMN_SELECT,
                                                        NULL);

    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                     GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 50);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new (); 
    column = gtk_tree_view_column_new_with_attributes ("User Name",
                                                        renderer,
                                                       "text",
                                                        COLUMN_USERNAME,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_USERNAME);
    gtk_tree_view_append_column (treeview, column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("User ID",
                                                        renderer,
                                                       "text",
                                                        COLUMN_USERID,
                                                        NULL);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_USERID);
    gtk_tree_view_append_column (treeview, column);

}

/******************************************************************************
* Function:             GetGroupInfo 
*        
* Explain:  Get information about all user groups
*        
* Input:         
*        
* Output:  A list of storage group objects
*        
* Author:  zhuyaliang  01/04/2019
******************************************************************************/
static GSList *GetGroupInfo(void)
{
    GasGroupManager *GroupManager;
    GSList *list, *l;
    GSList *GroupsList = NULL;
    int i = 0;
    int count = 0;
    UserGroup *usergroup;

    GroupManager = gas_group_manager_get_default ();
    if(GroupManager == NULL)
    {
        MessageReport(_("Initialization group management"),
                      _("Initialization failed, please see Group \n Management Service Interface function"),
                      ERROR);
        return NULL;
    }
    if( gas_group_manager_no_service(GroupManager) == TRUE)
    {
        MessageReport(_("Failed to contact the group service"),
                      _("Please make sure that the group-service is installed and enabled.\n url: https://github.com/zhuyaliang/group-service"),
                        ERROR);
        return  NULL;
    }
    list = gas_group_manager_list_groups (GroupManager);
    count = g_slist_length(list);
    if(count <= 0 )
    {
        MessageReport(_("Get the number of groups"),
                      _("The number of groups is 0."),
                      ERROR);
        return NULL;
    }
    for(l = list; l ; l = l->next,i++)
    {
        usergroup = GroupInit(l->data);
        if(usergroup != NULL)
        {
            GroupsList = g_slist_append(GroupsList,g_object_ref(usergroup));
        }
    }
    g_slist_free (list);
    return GroupsList;
}
static GtkWidget *CreateManageWindow(void)
{
    GtkWidget *Window;

    Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(Window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(Window), ("Groups Manage"));
    gtk_container_set_border_width(GTK_CONTAINER(Window),10);
    
    return Window;
} 
static GtkWidget *GetGridWidget (void)
{
    GtkWidget *table;
    
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
    
    return table;
}    
static GtkWidget *GetScrolledWidget (void)
{
    GtkWidget *Scrolled;
    
    Scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (Scrolled),
                                         GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (Scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);

    return Scrolled;
}
static GtkWidget *LoadSwitchGroup(GroupsManage *gm)
{
    GtkWidget *vbox;
    GtkWidget *vbox1;
    GtkWidget *Scrolled;
    GtkTreeModel *model;
    GtkWidget *treeview;
    GtkWidget *table;
    GtkWidget *ButtonClose;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
 
    vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (vbox1, -1,200);
    
    table = GetGridWidget();
    gtk_box_pack_start(GTK_BOX(vbox),table, TRUE, TRUE,0);
    Scrolled = GetScrolledWidget(); 
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);
    
    model    = CreateSwicthModel(gm->GroupsList,gm->username);
	gm->SwitchStore = GTK_LIST_STORE(model);	
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                     COLUMN_GROUPID);
    g_object_unref (model);
    gtk_container_add (GTK_CONTAINER (Scrolled), treeview);

    gm->TreeSwitch = treeview;
    AddSwitchGroupColumns (gm);
    
    gtk_grid_attach(GTK_GRID(table) , vbox1 , 0 , 0 , 3 , 1); 
    
    ButtonClose    =  gtk_button_new_with_label(("Close"));
    gtk_grid_attach(GTK_GRID(table) , ButtonClose , 1 , 1 , 1 , 1); 
    g_signal_connect (ButtonClose, 
                     "clicked",
                      G_CALLBACK (CloseGroupWindow),
                      gm);
 
    return vbox;
}

static GtkWidget *LoadCreateGroup(GroupsManage *gm,GSList *List)
{
    GtkWidget *vbox;
    GtkWidget *vbox1;
    GtkWidget *Scrolled;
    GtkWidget *treeview;
    GtkWidget *table;
    GtkWidget *GroupNameLabel;
    GtkWidget *TipsLabel;
    GtkWidget *ButtonClose;
    GtkWidget *ButtonConfirm;
    GtkTreeModel     *model;

    vbox  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (vbox1, -1,130);

    table = GetGridWidget();
    gtk_box_pack_start(GTK_BOX(vbox),table, TRUE, TRUE,0);
    
    GroupNameLabel = gtk_label_new(NULL);
    SetLableFontType(GroupNameLabel,"gray",10,("Group Name"));
    gtk_grid_attach(GTK_GRID(table) ,GroupNameLabel ,0,0,1 ,1);

    gm->EntryGroupName = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(gm->EntryGroupName),48);
    gtk_grid_attach(GTK_GRID(table) ,gm->EntryGroupName ,1,0,1 ,1);

    Scrolled = GetScrolledWidget();
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);
    
    TipsLabel = gtk_label_new(NULL);
    SetLableFontType(TipsLabel,"black",12,("Add Users To Group"));
    gtk_grid_attach(GTK_GRID(table) , TipsLabel ,0 ,1,1 ,1);

    model = CreateAddUsersModel(List);
    treeview = gtk_tree_view_new_with_model (model);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
                                     COLUMN_GROUPID);
    g_object_unref (model);
    gtk_container_add (GTK_CONTAINER (Scrolled), treeview);
    gm->TreeCreate = treeview;
    gm->NewGroupUsers = NULL;
    AddSelectUsersColumns (gm);
    gtk_grid_attach(GTK_GRID(table) ,vbox1 ,0 ,2,2 ,1);

    ButtonClose    =  gtk_button_new_with_label(("Close"));
    gtk_grid_attach(GTK_GRID(table) , ButtonClose , 0 , 3, 1 , 1);
    g_signal_connect (ButtonClose, 
                     "clicked",
                      G_CALLBACK (CloseGroupWindow),
                      gm);

    ButtonConfirm    =  gtk_button_new_with_label(("Confirm"));
    gtk_grid_attach(GTK_GRID(table) , ButtonConfirm , 1 , 3 , 1 , 1);
    g_signal_connect (ButtonConfirm, 
                     "clicked",
                      G_CALLBACK (CreateNewGroup),
                      gm);
    
    return vbox;
}
static GtkWidget *LoadRemoveGroup(GroupsManage *gm)
{
    GtkWidget *vbox;
//    GtkWidget *vbox1;
//    GtkWidget *Scrolled;
//    GtkTreeModel *model;
//    GtkWidget *treeview;
//    GtkWidget *table;
//    GtkWidget *ButtonClose;
//    GtkWidget *ButtonConfirm;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
   /*
    vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request (vbox1, -1,200);
    
    table = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(table),TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);

    gtk_box_pack_start(GTK_BOX(vbox),table, TRUE, TRUE,0);
    Scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (Scrolled),
                                         GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (Scrolled),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox1), Scrolled, TRUE, TRUE, 0);
 */
    return vbox;
}
static void StartManageGroups (GroupsManage *gm,GSList *UsersList)
{
    GtkWidget *NoteBook;
    GtkWidget *SwitchBox;
    GtkWidget *CreateBox;
    GtkWidget *RemoveBox;
    GtkWidget *SwitchNoteName;
    GtkWidget *CreateNoteName;
    GtkWidget *RemoveNoteName;
    const gchar *CurrentUserName;
    UserInfo  *user;
    
    user = GetIndexUser(UsersList,gnCurrentUserIndex);
    CurrentUserName = GetUserName(user->ActUser);
    gm->username = g_strdup(CurrentUserName);

    NoteBook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(gm->GroupsWindow), NoteBook);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (NoteBook), GTK_POS_TOP);

    SwitchNoteName = gtk_label_new(("Switch Groups"));
    SwitchBox = LoadSwitchGroup(gm);
    gtk_notebook_append_page(GTK_NOTEBOOK (NoteBook),SwitchBox,SwitchNoteName);

    CreateNoteName = gtk_label_new(("Create Groups"));
    CreateBox = LoadCreateGroup(gm,UsersList);
    gtk_notebook_append_page(GTK_NOTEBOOK (NoteBook),CreateBox,CreateNoteName);

    RemoveNoteName = gtk_label_new(("Remove Groups"));
    RemoveBox = LoadRemoveGroup(gm);
    gtk_notebook_append_page(GTK_NOTEBOOK (NoteBook),RemoveBox,RemoveNoteName);

    gtk_widget_show_all(gm->GroupsWindow);
}    
/******************************************************************************
* Function:             UserGroupsManage 
*        
* Explain: User Group Management Callback Function
*        
* Input:         
*        
* Output: 
*        
* Author:  zhuyaliang  01/04/2019
******************************************************************************/
void UserGroupsManage (GtkWidget *widget, gpointer data)
{
    UserAdmin *ua = (UserAdmin *)data;
	gtk_widget_hide(WindowLogin);
   	ua->gm.GroupsList = NULL;
    ua->gm.GroupsList = GetGroupInfo();
    if(ua->gm.GroupsList == NULL)
    {
		gtk_widget_show(WindowLogin);
        return;
    }  
    ua->gm.GroupNum = g_slist_length(ua->gm.GroupsList);  
    ua->gm.GroupsWindow = CreateManageWindow();
    StartManageGroups(&ua->gm,ua->UsersList);
    g_signal_connect(G_OBJECT(ua->gm.GroupsWindow), 
                    "delete-event",
                     G_CALLBACK(QuitGroupWindow),
                     &ua->gm);
}    
