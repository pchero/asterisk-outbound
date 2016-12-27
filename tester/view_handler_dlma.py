# -*- coding: utf-8 -*-
"""
Created on Sat Dec 31 21:28:29 2016

@author: pchero
"""


import Tkinter as tk
import ttk
import tkFont
import tkSimpleDialog

class FrameMain(object):
    container = None
    action_handler = None
    data_handler = None
        
    # info
    list_headers = ["uuid"]
    detail_headers = ["key", "value"]
    list_tree = None
    list_items = None
    detail_tree = None
    detail_items = None
    
    # sub info
    sub_list_headers = ["uuid"]
    sub_detail_headers = ["key", "value"]
    sub_list_tree = None
    sub_list_items = None
    sub_detail_tree = None
    sub_detail_items = None
    
    
    # activated items
    activated_list_items = None
    
    def __init__(self, master, data_handler, control_handler):
        # set handlers
        self.data_handler = data_handler
        self.data_handler.set_view_handler(self)
        
        self.action_handler = control_handler
        self.action_handler.set_veiw_handler(self)
        
        self.container = tk.Frame(master)
        self.container.grid()
        self.frame_setup()
        
        
    def destroy(self):
        print("destroy")
        self.container.destroy()
            
        
    def frame_setup(self):
        self.frame_main()
        self.update_list_items("dlma")
        return
    

    def frame_main(self):
        frame = tk.Frame(self.container)
        frame.grid()
        frame.grid_rowconfigure(0, weight=1)
        frame.grid_columnconfigure(0, weight=1)

        # create list treeview
        list_tree = ttk.Treeview(frame, columns=self.list_headers, show="headings", height=15)
        list_tree.grid(column=0, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        list_vsb = ttk.Scrollbar(frame, orient="vertical", command=list_tree.yview)
        list_vsb.grid(column=1, row=0, sticky='ns', rowspan=4)
        list_tree.configure(yscrollcommand=list_vsb.set)
        #list_tree.bind("<Double-Button-1>", self.action_handler.list_view_handler)
        list_tree.bind("<Double-Button-1>", self._action_list_double_click)
        self.list_tree = list_tree
        
        # create detail treeview
        detail_tree = ttk.Treeview(frame, columns=self.detail_headers, show="headings", height=15)
        detail_tree.grid(column=2, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        detail_vsb = ttk.Scrollbar(frame, orient="vertical", command=detail_tree.yview)
        detail_vsb.grid(column=3, row=0, sticky='ns', rowspan=4)
        detail_tree.configure(yscrollcommand=detail_vsb.set)
        detail_tree.bind("<Double-Button-1>", self._action_detail_double_click)
        self.detail_tree = detail_tree
        
        # Buttons
        bt_show = tk.Button(frame, text="Show", width=8, command=self._action_button_show)
        bt_show.grid(column=4, row=0, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_create = tk.Button(frame, text="Create", width=8, command=self._action_button_create)
        bt_create.grid(column=4, row=1, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_update = tk.Button(frame, text="Update", width=8, command=self._action_button_update)
        bt_update.grid(column=4, row=2, sticky=tk.E+tk.W+tk.N+tk.S)
        
        bt_delete = tk.Button(frame, text="Delete", width=8, command=self._action_button_delete)
        bt_delete.grid(column=4, row=3, sticky=tk.E+tk.W+tk.N+tk.S)
        
        
        # create sub list treeview
        self.sub_list_tree = ttk.Treeview(frame, columns=self.sub_list_headers, show="headings", height=15)
        self.sub_list_tree.grid(column=0, row=4, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        list_vsb = ttk.Scrollbar(frame, orient="vertical", command=self.sub_list_tree.yview)
        list_vsb.grid(column=1, row=4, sticky='ns', rowspan=4)
        self.sub_list_tree.configure(yscrollcommand=list_vsb.set)
        #list_tree.bind("<Double-Button-1>", self.action_handler.list_view_handler)
        self.sub_list_tree.bind("<Double-Button-1>", self._action_sub_list_double_click)
        
        # create sub detail treeview
        self.sub_detail_tree = ttk.Treeview(frame, columns=self.sub_detail_headers, show="headings", height=15)
        self.sub_detail_tree.grid(column=2, row=4, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        detail_vsb = ttk.Scrollbar(frame, orient="vertical", command=self.sub_detail_tree.yview)
        detail_vsb.grid(column=3, row=4, sticky='ns', rowspan=4)
        self.sub_detail_tree.configure(yscrollcommand=detail_vsb.set)
        self.sub_detail_tree.bind("<Double-Button-1>", self._action_sub_detail_double_click)

        bt_diallist_show = tk.Button(frame, text="DialList Show", width=8, command=self._action_button_diallist_show)
        bt_diallist_show.grid(column=4, row=4, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_diallist_create = tk.Button(frame, text="DialList Create", width=8, command=self._action_button_diallist_create)
        bt_diallist_create.grid(column=4, row=5, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_diallist_update = tk.Button(frame, text="DialList Update", width=8, command=self._action_button_diallist_update)
        bt_diallist_update.grid(column=4, row=6, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_diallist_delete = tk.Button(frame, text="DialList Delete", width=8, command=self._action_button_diallist_delete)
        bt_diallist_delete.grid(column=4, row=7, sticky=tk.E+tk.W+tk.N+tk.S)


    def _get_list_activated_items(self):
        print("_get_list_activated_items")
        
        cur_item = self.list_tree.focus()
        item = self.list_tree.item(cur_item)["values"]
        return item
        

    def _action_list_double_click(self, event):
        print("_action_list_double_click")
        
        # get activated item
        # get selected key, value
        item = self._get_list_activated_items()
        uuid = item[0]
        
        self.update_detail_items(uuid)
        
        # get diallist item
        self.sub_list_items = self.data_handler.dlma_get_diallist_list_all(uuid)
        print("sub_list_item. item[%s]" % (self.sub_list_items))
        
        # get dial list
        self.update_sub_list_tree()
        
        return
        
    
    def _action_detail_double_click(self, event):
        print("_action_detail_double_click")

        # get selected key, value        
        cur_item = self.detail_tree.focus()
        key = self.detail_tree.item(cur_item)["values"][0]
        value = self.detail_tree.item(cur_item)["values"][1]
        print("key, value. key[%s], value[%s]" % (key, value))

        # get new value
        ret = tkSimpleDialog.askstring("New value", "Please enter a new value")
        if ret == None:
            return
            
        # update detail
        print ("result. ret[%s]" % (ret))
        self.detail_items[key] = ret
        
        self.update_detail()
            
        return
        
    
    def _action_button_show(self):
        print("_action_button_show")
        # get search uuid
        ret = tkSimpleDialog.askstring("Show dlma", "Please enter a dlma uuid")
        if ret == None:
            return
        
        if ret == "":
            self.action_handler.send_cmd_async("OutDlmaShow")
        else:
            data = {"Uuid":ret}
            self.action_handler.send_cmd_async("OutDlmaShow", data)
        return

    
    def _action_button_create(self):
        print("_action_button_create")
        
        self.action_handler.send_cmd_async("OutDlmaCreate")
        return
        
        
    def _action_button_update(self):
        print("_action_button_update")
        items = self.detail_items
        
        self.action_handler.send_cmd_async("OutDlmaUpdate", items)
        return

    
    def _action_button_delete(self):
        print("_action_button_delete")
        items = self.detail_items
        
        uuid = items.pop("Uuid", None)
        if uuid == None:
            print("Could not get uuid info. item[%s]", items)
            return
        
        data = {"Uuid":uuid}
        self.action_handler.send_cmd_async("OutDlmaDelete", data)
        return


    def _update_list(self):
        print("_update_list")

        # delete all items
        for i in self.list_tree.get_children():
            self.list_tree.delete(i)

        items = self.list_items                

        # insert items
        for col in self.list_headers:
            self.list_tree.heading(col, text=col.title(), command=lambda c=col: sortby(self.list_tree, c, 0))
            # adjust the column's width to the header string
            self.list_tree.column(col, width=tkFont.Font().measure(col.title()))
        
        # insert imters
        for key in items:
            self.list_tree.insert('', 'end', values=(key))

            # size arrange
            col_w = tkFont.Font().measure(key)
            if self.list_tree.column(self.list_headers[0], width=None) < col_w:
                self.list_tree.column(self.list_headers[0], width=col_w)
                

    def update_list_items(self, table):
        '''
        '''
        print("update_list_items")

        if table == None or table != "dlma":
            return        
        
        self.list_items = self.data_handler.dlma_get_list_all()
        
        self._update_list()


    def update_detail_items(self, uuid):
    
        if uuid == None:
            return
        
        data = self.data_handler.dlma_get(uuid)
        if data == None:
            print("Could not find correct dlma info. uuid[%s]" % uuid)
            return
        self.detail_items = data.copy()
        
        self.update_detail()
        
        return
    
    
    def update_detail(self):
        '''
        update the detail tree
        '''
        items = self.detail_items

        # delete all items
        for i in self.detail_tree.get_children():
            self.detail_tree.delete(i)
        
        # sort
        for col in self.detail_headers:
            self.detail_tree.heading(col, text=col.title(), command=lambda c=col: sortby(self.detail_tree, c, 0))
            # adjust the column's width to the header string
            self.detail_tree.column(col, width=tkFont.Font().measure(col.title()))
            
        if items == None:
            return

        # insert items
        for key, val in items.iteritems():
            self.detail_tree.insert('', 'end', values=(key, val))

            # size arrange
            col_w = tkFont.Font().measure(key)
            if self.detail_tree.column(self.detail_headers[0], width=None) < col_w:
                self.detail_tree.column(self.detail_headers[0], width=col_w)
            
            col_w = tkFont.Font().measure(val)
            if self.detail_tree.column(self.detail_headers[1], width=None) < col_w:
                self.detail_tree.column(self.detail_headers[1], width=col_w)
        return


    def update_detail_item(self, event):
        print("OnClick detail")

        # get selected key, value        
        cur_item = self.detail_tree.focus()
        key = self.detail_tree.item(cur_item)["values"][0]
        value = self.detail_tree.item(cur_item)["values"][1]
        print("key, value. key[%s], value[%s]" % (key, value))

        # get new value
        ret = tkSimpleDialog.askstring("New value", "Please enter a new value")
        if ret == None:
            return
            
        # update 
        print ("result. ret[%s]" % (ret))
        self.detail_items[key] = ret
        
        self.update_detail()
            
        return
        
        
    def _get_sub_list_activated_items(self):
        print("_get_list_activated_items")
        
        cur_item = self.sub_list_tree.focus()
        item = self.sub_list_tree.item(cur_item)["values"]
        return item
        
        
    def _get_sub_detail_activated_items(self):
        print("_get_sub_detail_activated_items")
        
        cur_item = self.sub_detail_tree.focus()
        item = self.sub_detail_tree.item(cur_item)["values"]
        return item
        

    def _action_sub_list_double_click(self, event):
        print("_action_list_double_click")
        
        # get activated item
        # get selected key, value
        item = self._get_sub_list_activated_items()
        uuid = item[0]
        
        self.update_sub_detail_items(uuid)
                
        return

        
    def _action_sub_detail_double_click(self, event):
        print("_action_sub_detail_double_click")

        # get selected key, value     
        item = self._get_sub_detail_activated_items()
        key = item[0]
        value = item[1]
        print("key, value. key[%s], value[%s]" % (key, value))

        # get new value
        ret = tkSimpleDialog.askstring("New value", "Please enter a new value")
        if ret == None:
            return
            
        # update detail
        print ("result. ret[%s]" % (ret))
        self.sub_detail_items[key] = ret
        
        self.update_sub_detail()
            
        return

    
    def _action_button_diallist_show(self):
        print("_action_button_diallist_show")
        # get search uuid
        
        items = self._get_list_activated_items()
        if items == None:
            return
        
        dlma_uuid = items[0]
        data = {"DlmaUuid":str(dlma_uuid)}
        print("Get dlma info. dlma_uuid[%s]" % dlma_uuid)
        
        ret = tkSimpleDialog.askinteger("Show diallist", "Please enter a count")
        if ret == None:
            return
        
        if ret != 0:
            data["Count"] = ret.__str__()
        
        self.action_handler.send_cmd_async("OutDlListShow", data)        
        return
    
    
    def _action_button_diallist_create(self):
        print("_action_button_diallist_create")
        
        self.action_handler.send_cmd_async("OutDlListCreate")
        return


    def _action_button_diallist_update(self):
        print("_action_button_diallist_update")
        items = self.sub_detail_items
        
        self.action_handler.send_cmd_async("OutDlListUpdate", items)
        return

    
    def _action_button_diallist_delete(self):
        print("_action_button_diallist_delete")
        items = self.sub_detail_items
        
        uuid = items.pop("Uuid", None)
        if uuid == None:
            print("Could not get uuid info. item[%s]", items)
            return
        
        data = {"Uuid":uuid}
        self.action_handler.send_cmd_async("OutDlListDelete", data)
        return


    def update_sub_list_tree(self):
        print("update_sub_list_tree")
        items = self.sub_list_items

        # delete all items
        for i in self.sub_list_tree.get_children():
            self.sub_list_tree.delete(i)

        for col in self.sub_list_headers:
            self.sub_list_tree.heading(col, text=col.title(), command=lambda c=col: sortby(self.sub_list_tree, c, 0))
            # adjust the column's width to the header string
            self.sub_list_tree.column(col, width=tkFont.Font().measure(col.title()))

        if items == None:
            return
            
        # insert items
        #for item in items.iteritems():
        for item in items:
            self.sub_list_tree.insert('', 'end', values=(item))

            # size arrange
            col_w = tkFont.Font().measure(item)
            if self.sub_list_tree.column(self.sub_list_headers[0], width=None) < col_w:
                self.sub_list_tree.column(self.sub_list_headers[0], width=col_w)
            
        return
            

    def update_sub_detail_items(self, uuid):
            
        if uuid == None:
            return
        
        data = self.data_handler.diallist_get(uuid)
        if data == None:
            print("Could not find correct dlma info. uuid[%s]" % uuid)
            return
        self.sub_detail_items = data
        
        self.update_sub_detail()
        
        return


    def update_sub_detail(self):
        '''
        update the sub detail tree
        '''
        items = self.sub_detail_items

        # delete all items
        for i in self.sub_detail_tree.get_children():
            self.sub_detail_tree.delete(i)
        
        # sort
        for col in self.sub_detail_headers:
            self.sub_detail_tree.heading(col, text=col.title(), command=lambda c=col: sortby(self.sub_detail_tree, c, 0))
            # adjust the column's width to the header string
            self.sub_detail_tree.column(col, width=tkFont.Font().measure(col.title()))
            
        if items == None:
            return

        # insert items
        for key, val in items.iteritems():
            self.sub_detail_tree.insert('', 'end', values=(key, val))

            # size arrange
            col_w = tkFont.Font().measure(key)
            if self.sub_detail_tree.column(self.sub_detail_headers[0], width=None) < col_w:
                self.sub_detail_tree.column(self.sub_detail_headers[0], width=col_w)
            
            col_w = tkFont.Font().measure(val)
            if self.sub_detail_tree.column(self.sub_detail_headers[1], width=None) < col_w:
                self.sub_detail_tree.column(self.sub_detail_headers[1], width=col_w)
        return
