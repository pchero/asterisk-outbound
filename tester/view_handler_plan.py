# -*- coding: utf-8 -*-
"""
Created on Fri Dec 30 23:36:33 2016

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
        
    # plan
    plan_list_headers = ["uuid"]
    plan_detail_headers = ["key", "value"]
    plan_list_tree = None
    plan_list_items = None
    plan_detail_tree = None
    plan_detail_items = None
    
    
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
        self.container.destroy()
            
        
    def frame_setup(self):
        self.frame_main_plan()
        self.update_list_items("plan")
        return
    

    def frame_main_plan(self):
        frame = tk.Frame(self.container)
        frame.grid()
        frame.grid_rowconfigure(0, weight=1)
        frame.grid_columnconfigure(0, weight=1)

        # create list treeview
        list_tree = ttk.Treeview(frame, columns=self.plan_list_headers, show="headings", height=30)
        list_tree.grid(column=0, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        list_vsb = ttk.Scrollbar(frame, orient="vertical", command=list_tree.yview)
        list_vsb.grid(column=1, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        list_tree.configure(yscrollcommand=list_vsb.set)
        list_tree.bind("<Double-Button-1>", self._action_plan_list_double_click)
        self.plan_list_tree = list_tree
        
        # create detail treeview
        detail_tree = ttk.Treeview(frame, columns=self.plan_detail_headers, show="headings", height=30)
        detail_tree.grid(column=2, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        detail_vsb = ttk.Scrollbar(frame, orient="vertical", command=detail_tree.yview)
        detail_vsb.grid(column=3, row=0, sticky=tk.E+tk.W+tk.N+tk.S, rowspan=4)
        detail_tree.configure(yscrollcommand=detail_vsb.set)        
        detail_tree.bind("<Double-Button-1>", self._action_plan_detail_double_click)        
        self.plan_detail_tree = detail_tree
        
        # Buttons
        bt_create = tk.Button(frame, text="Show", width=8, command=self._action_button_plan_show)
        bt_create.grid(column=4, row=0, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_create = tk.Button(frame, text="Create", width=8, command=self._action_button_plan_create)
        bt_create.grid(column=4, row=1, sticky=tk.E+tk.W+tk.N+tk.S)

        bt_update = tk.Button(frame, text="Update", width=8, command=self._action_button_plan_update)
        bt_update.grid(column=4, row=2, sticky=tk.E+tk.W+tk.N+tk.S)
        
        bt_delete = tk.Button(frame, text="Delete", width=8, command=self._action_button_plan_delete)
        bt_delete.grid(column=4, row=3, sticky=tk.E+tk.W+tk.N+tk.S)


    def _action_plan_list_double_click(self, event):
        # get activated item
        # get selected key, value
        cur_item = self.plan_list_tree.focus()
        plan_uuid = self.plan_list_tree.item(cur_item)["values"][0]
        
        self.update_plan_detail_items(plan_uuid)
        return
        
    
    def _action_plan_detail_double_click(self, event):
        print("_event_plan_detail_double_click")

        # get selected key, value        
        cur_item = self.plan_detail_tree.focus()
        key = self.plan_detail_tree.item(cur_item)["values"][0]
        value = self.plan_detail_tree.item(cur_item)["values"][1]
        print("key, value. key[%s], value[%s]" % (key, value))

        # get new value
        ret = tkSimpleDialog.askstring("New value", "Please enter a new value", initialvalue=value)
        if ret == None:
            return
            
        # update 
        print ("result. ret[%s]" % (ret))
        self.plan_detail_items[key] = ret
        
        self.update_plan_detail()
            
        return
        
    
    def _action_button_plan_show(self):
        print("_action_button_plan_show")
        # get search uuid
        ret = tkSimpleDialog.askstring("Show plan", "Please enter a plan uuid")
        if ret == None:
            return
        
        if ret == "":
            self.action_handler.send_cmd_async("OutPlanShow")
        else:
            data = {"Uuid":ret}
            self.action_handler.send_cmd_async("OutPlanShow", data)
        return

    
    def _action_button_plan_create(self):
        print("_action_button_plan_create")
        
        self.action_handler.send_cmd_async("OutPlanCreate")
        return
        
        
    def _action_button_plan_update(self):
        print("_action_button_plan_update")
        items = self.plan_detail_items
        
        self.action_handler.send_cmd_async("OutPlanUpdate", items)
        return

    
    def _action_button_plan_delete(self):
        print("_action_button_plan_delete")
        items = self.plan_detail_items
        
        uuid = items.pop("Uuid", None)
        if uuid == None:
            print("Could not get uuid info. item[%s]", items)
            return
        
        data = {"Uuid":uuid}
        self.action_handler.send_cmd_async("OutPlanDelete", data)
        return
            

    def _update_plan_list(self):
        print("_update_plan_list")
        
        # delete all items
        for i in self.plan_list_tree.get_children():
            self.plan_list_tree.delete(i)

        items = self.plan_list_items                

        # insert items        
        for col in self.plan_list_headers:
            self.plan_list_tree.heading(col, text=col.title(), command=lambda c=col: sortby(self.plan_list_tree, c, 0))
            # adjust the column's width to the header string
            self.plan_list_tree.column(col, width=tkFont.Font().measure(col.title()))
        
        # insert imters
        for key in items:
            self.plan_list_tree.insert('', 'end', values=(key))

            # size arrange
            col_w = tkFont.Font().measure(key)
            if self.plan_list_tree.column(self.plan_list_headers[0], width=None) < col_w:
                self.plan_list_tree.column(self.plan_list_headers[0], width=col_w)
                

    def update_list_items(self, table):
        '''
        '''
        print("update_plan_list_items")

        if table == None or table != "plan":
            return        
        
        self.plan_list_items = self.data_handler.plan_get_list_all()
        
        #print self.plan_list_items
        self._update_plan_list()

    
    def update_plan_detail_items(self, uuid):
    
        if uuid == None:
            return
        
        data = self.data_handler.plan_get(uuid)
        if data == None:
            print("Could not find correct plan info. uuid[%s]" % uuid)
            return
        print data
        self.plan_detail_items = data
        
        self.update_plan_detail()
        
        return
    
    
    def update_plan_detail(self):
        '''
        update the plan detail tree
        '''
        items = self.plan_detail_items

        # delete all items
        for i in self.plan_detail_tree.get_children():
            self.plan_detail_tree.delete(i)
        
        # sort
        for col in self.plan_detail_headers:
            self.plan_detail_tree.heading(col, text=col.title(), command=lambda c=col: sortby(self.plan_detail_tree, c, 0))
            # adjust the column's width to the header string
            self.plan_detail_tree.column(col, width=tkFont.Font().measure(col.title()))
            
        if items == None:
            return

        # insert items
        for key, val in items.iteritems():
            self.plan_detail_tree.insert('', 'end', values=(key, val))

            # size arrange
            col_w = tkFont.Font().measure(key)
            if self.plan_detail_tree.column(self.plan_detail_headers[0], width=None) < col_w:
                self.plan_detail_tree.column(self.plan_detail_headers[0], width=col_w)
            
            col_w = tkFont.Font().measure(val)
            if self.plan_detail_tree.column(self.plan_detail_headers[1], width=None) < col_w:
                self.plan_detail_tree.column(self.plan_detail_headers[1], width=col_w)
        return


    def update_plan_detail_item(self, event):
        print("OnClick detail")

        # get selected key, value        
        cur_item = self.plan_detail_tree.focus()
        key = self.plan_detail_tree.item(cur_item)["values"][0]
        value = self.plan_detail_tree.item(cur_item)["values"][1]
        print("key, value. key[%s], value[%s]" % (key, value))

        # get new value
        ret = tkSimpleDialog.askstring("New value", "Please enter a new value")
        if ret == None:
            return
            
        # update 
        print ("result. ret[%s]" % (ret))
        self.plan_detail_items[key] = ret
        
        self.update_plan_detail()
            
        return
