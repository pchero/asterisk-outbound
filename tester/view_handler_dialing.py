# -*- coding: utf-8 -*-
"""
Created on Fri Jan 13 21:00:38 2017

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
        self.update_list_items("dialing")
        return
    

    def frame_main(self):
        frame = tk.Frame(self.container)
        frame.grid()
        frame.grid_rowconfigure(0, weight=1)
        frame.grid_columnconfigure(0, weight=1)

        # create list treeview
        self.list_tree = ttk.Treeview(frame, columns=self.list_headers, show="headings", height=30)                
        self.list_tree.grid(column=0, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        list_vsb = ttk.Scrollbar(frame, orient="vertical", command=self.list_tree.yview)
        list_vsb.grid(column=1, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        self.list_tree.configure(yscrollcommand=list_vsb.set)
        self.list_tree.bind("<Double-Button-1>", self._action_list_double_click)
        
        # create detail treeview
        detail_tree = ttk.Treeview(frame, columns=self.detail_headers, show="headings", height=30)
        detail_tree.grid(column=2, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        detail_vsb = ttk.Scrollbar(frame, orient="vertical", command=detail_tree.yview)
        detail_vsb.grid(column=3, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        detail_tree.configure(yscrollcommand=detail_vsb.set)
        detail_tree.bind("<Double-Button-1>", self._action_detail_double_click)
        self.detail_tree = detail_tree
        
        # Buttons
        bt_create = tk.Button(frame, text="Show", width=8, command=self._action_button_show)
        bt_create.grid(column=4, row=0, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_create = tk.Button(frame, text="Create", width=8, command=self._action_button_create)
        bt_create.grid(column=4, row=1, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_update = tk.Button(frame, text="Update", width=8, command=self._action_button_update)
        bt_update.grid(column=4, row=2, sticky=tk.E+tk.W+tk.N+tk.S)
        
        bt_delete = tk.Button(frame, text="Delete", width=8, command=self._action_button_delete)
        bt_delete.grid(column=4, row=3, sticky=tk.E+tk.W+tk.N+tk.S)


    def _action_list_double_click(self, event):
        # get activated item
        # get selected key, value
        cur_item = self.list_tree.focus()
        uuid = self.list_tree.item(cur_item)["values"][0]
        
        self.update_detail_items(uuid)
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
            
        # update 
        print ("result. ret[%s]" % (ret))
        self.detail_items[key] = ret
        
        self.update_detail()
            
        return
        
    
    def _action_button_show(self):
        print("_action_button_show")
        # get search uuid
        ret = tkSimpleDialog.askstring("Show dialing", "Please enter a dialing uuid")
        if ret == None:
            return
        
        if ret == "":
            self.action_handler.send_cmd_async("OutDialingShow")
        else:
            data = {"Uuid":ret}
            self.action_handler.send_cmd_async("OutDialingShow", data)
        return

    
    def _action_button_create(self):
        print("_action_button_create")
        
        self.action_handler.send_cmd_async("OutDialingCreate")
        return
        
        
    def _action_button_update(self):
        print("_action_button_update")
        items = self.detail_items
        
        self.action_handler.send_cmd_async("OutDialingUpdate", items)
        return

    
    def _action_button_delete(self):
        print("_action_button_delete")
        items = self.detail_items
        
        uuid = items.pop("Uuid", None)
        if uuid == None:
            print("Could not get uuid info. item[%s]", items)
            return
        
        data = {"Uuid":uuid}
        self.action_handler.send_cmd_async("OutDialingDelete", data)
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
                

    def update_list_items(self, items):
        '''
        '''
        print("update_list_items")
        if items == None or items != "dialing":
            return
        
        self.list_items = self.data_handler.dialing_get_list_all()
        #print self.list_items
        self._update_list()

    
    def update_detail_items(self, uuid):
    
        if uuid == None:
            return
        
        data = self.data_handler.dialing_get(uuid)
        if data == None:
            print("Could not find correct dialing info. uuid[%s]" % uuid)
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
