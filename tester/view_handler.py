# -*- coding: utf-8 -*-
"""
Created on Tue Dec 27 20:23:29 2016

@author: pchero
"""

import Tkinter as tk

import control_handler
import data_handler
import view_handler_plan
import view_handler_campaign
import view_handler_dlma
import view_handler_destination
import view_handler_dialing



def isnumeric(s):
    """test if a string is numeric"""
    for c in s:
        if c in "1234567890-.":
            numeric = True
        else:
            return False
    return numeric


def change_numeric(data):
    """if the data to be sorted is numeric change to float"""
    new_data = []
    if isnumeric(data[0][0]):
        # change child to a float
        for child, col in data:
            new_data.append((float(child), col))
        return new_data
    return data


def sortby(tree, col, descending):
    """sort tree contents when a column header is clicked on"""
    # grab values to sort
    data = [(tree.set(child, col), child) for child in tree.get_children('')]
    # if the data to be sorted is numeric change to float
    data = change_numeric(data)
    # now sort the data in place
    data.sort(reverse=descending)
    for ix, item in enumerate(data):
        tree.move(item[1], '', ix)
    # switch the heading so that it will sort in the opposite direction
    tree.heading(col, command=lambda col=col: sortby(tree, col, int(not descending)))


class MainFrame(object):
    container = None
    action_handler = None
    data_handler = None
    
    # login
    login_entry_server_ip = None
    login_entry_server_port = None
    login_entry_username = None
    login_entry_password = None
    
    # select list
    select_list = [
        ("Campaign", 1), 
        ("Plan", 2), 
        ("Destination", 3), 
        ("DialMaster", 4), 
        ("Dialing", 5)
        ]
    selected_frame = None
    
    def __init__(self, master):
        self.selected_frame = tk.IntVar()        
        
        # set handlers
        self.data_handler = data_handler.DataHandler()
        #self.data_handler.set_view_handler(self)
        self.action_handler = control_handler.MainControl()
        self.action_handler.set_data_handler(self.data_handler)
        
        self.container = tk.Frame(master)
        self._recv_data()
        self.container.grid()
        self.frame_setup()
    
    
    def _recv_data(self):
        ret = self.action_handler.recv_data()
        if ret == False:
            self.container.after(1000, self._recv_data)
        else:
            self.container.after(0, self._recv_data)
        
        
    def frame_setup(self):
        self.frame_login()
        #self.frame_main_plan()
        self.frame_main()
        self.frame_select()
        return
        
        
    def frame_login(self):
        frame = tk.Frame(self.container)
        frame.grid(row=0)
        
        row = 0
        col = 0        
        
        # entry server ip
        col = 0
        lb_server_ip = tk.Label(frame, text="Server ip", width=15)
        lb_server_ip.grid(row=0, column=col)
        col += 1
        self.entry_server_ip = tk.Entry(frame)
        self.entry_server_ip.grid(row=row, column=col)
        self.entry_server_ip.insert(0, "192.168.200.10")
        
        # entry server port
        col += 1
        lb_server_port = tk.Label(frame, text="Server port", width=15)
        lb_server_port.grid(row=row, column=col)
        col += 1
        self.entry_server_port = tk.Entry(frame)
        self.entry_server_port.grid(row=row, column=col)
        self.entry_server_port.insert(0, "5038")
    
        # entry username
        col += 1
        lb_username = tk.Label(frame, text="Username", width=15)
        lb_username.grid(row=row, column=col)
        col += 1
        self.entry_username = tk.Entry(frame)
        self.entry_username.grid(row=row, column=col)
        self.entry_username.insert(0, "admin")
        
        # entry password
        col += 1
        lb_password = tk.Label(frame, text="Password", width=15)
        lb_password.grid(row=row, column=col)
        col += 1
        self.entry_password = tk.Entry(frame)
        self.entry_password.grid(row=row, column=col)
        self.entry_password.insert(0, "admin")
        
        # button login
        col += 1
        bt_login = tk.Button(frame, text="Login", command=self._event_login_button_click)
        bt_login.grid(row=row, column=col)


    def frame_select(self):
        frame_select = tk.Frame(self.container)
        frame_select.grid(row=1)
        frame_select.grid_rowconfigure(0, weight=1)
        frame_select.grid_columnconfigure(0, weight=1)
        
        col = 0
        self.selected_frame.set(0)
        for txt, val in self.select_list:
            tk.Radiobutton(frame_select, text=txt, padx=20, variable=self.selected_frame, value=val, command=self._event_frame_select).grid(column=col, row=0)
            col += 1
            #tk.Radiobutton(frame_select, text=txt, padx=20).pack(anchor=tk.W)
        return


    def frame_main(self):
        
        frame_main = tk.Frame(self.container)
        frame_main.grid(row=2)
        frame_main.grid_rowconfigure(0, weight=1)
        frame_main.grid_columnconfigure(0, weight=1)
        
        self.frame_main = view_handler_campaign.FrameMain(frame_main, self.data_handler, self.action_handler)


    def _get_initial_info(self):
        self.action_handler.send_cmd_async("OutCampaignShow")
        self.action_handler.send_cmd_async("OutPlanShow")
        self.action_handler.send_cmd_async("OutDestinationShow")
        self.action_handler.send_cmd_async("OutDlmaShow")
        self.action_handler.send_cmd_async("OutDlListShow", {"Count": 1000000})
        
    
    def _event_login_button_click(self):
        
        # login
        self.action_handler.login_handler(
            self.entry_server_ip.get(), 
            self.entry_server_port.get(), 
            self.entry_username.get(), 
            self.entry_password.get()
            )
            
        self._get_initial_info()

        
    def _event_frame_select(self):
        frame_main = tk.Frame(self.container)
        frame_main.grid(row=2)
        frame_main.grid_rowconfigure(0, weight=1)
        frame_main.grid_columnconfigure(0, weight=1)
        
        try:
            self.frame_main.destroy()
        except:
            print("Error!")
        
        try:
            del self.frame_main
        except:
            print("Error!")

        frame_num = self.selected_frame.get()
        
        if frame_num == 1:
            self.frame_main = view_handler_campaign.FrameMain(frame_main, self.data_handler, self.action_handler)
        elif frame_num == 2:
            self.frame_main = view_handler_plan.FrameMain(frame_main, self.data_handler, self.action_handler)
        elif frame_num == 3:
            self.frame_main = view_handler_destination.FrameMain(frame_main, self.data_handler, self.action_handler)
        elif frame_num == 4:
            self.frame_main = view_handler_dlma.FrameMain(frame_main, self.data_handler, self.action_handler)
        elif frame_num == 5:
            self.frame_main = view_handler_dialing.FrameMain(frame_main, self.data_handler, self.action_handler)
        return

