# -*- coding: utf-8 -*-
"""
 Created on: Jan 13, 2017

@author: pchero
"""

import Tkinter as tk
import view_handler

def main():
    root = tk.Tk()
    
    main_frame = view_handler.MainFrame(root)
    root.mainloop()

if __name__ == '__main__':
    main()
