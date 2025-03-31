extern crate alloc;

use core::cell::OnceCell;
use alloc::{sync::Arc, vec::Vec};

use spin::RwLock;

use crate::screen::Screen;

pub static mut GLOBAL_SCREENS: RwLock<OnceCell<Manager<'static>>> = RwLock::new(OnceCell::new());
pub static mut GLOBAL_SCREEN_ID: RwLock<usize> = RwLock::new(0);

pub struct Manager<'scr> {
    screens: Vec<Screen>,
    default_screen: Option<&'scr mut Screen>
}

impl Manager<'static> {
    pub fn new() -> Self {
        Self {
            screens: Vec::new(),
            default_screen: None
        }
    }

    pub fn add_screen(&mut self, mut screen: Screen) -> usize {
        screen.global_id = Some(unsafe { *GLOBAL_SCREEN_ID.read() });

        self.screens.push(screen);

        self.screens.len() - 1
    }

    pub fn reselect(&'static mut self) {
        if self.screens.is_empty() {
            return;
        }

        self.default_screen = Some(&mut self.screens[0]);
    }

    pub fn default_screen(&self) -> Option<&Screen> {
        self.default_screen.as_deref()
    }

    pub fn default_screen_index(&self) -> Option<usize> {
        if self.default_screen.is_none() {
            return None;
        }

        self.screens.iter().position(|a| a.global_id == self.default_screen.as_ref().unwrap().global_id)
    }

    pub fn default_screen_mut(&mut self) -> Option<&mut Screen> {
        self.default_screen.as_deref_mut()
    }

    pub fn screens(&self) -> &Vec<Screen> {
        &self.screens
    }

    pub fn nth_screen(&self, index: usize) -> Option<&Screen> {
        self.screens.get(index)
    }

    pub fn nth_screen_mut(&mut self, index: usize) -> Option<&mut Screen> {
        self.screens.get_mut(index)
    }
}

pub fn with_manager<F, R>(f: F) -> R
where F: Fn(&Manager) -> R {
    let mgr_ref = unsafe { &GLOBAL_SCREENS };

    let guard = mgr_ref.read();
    let mgr = guard.get().unwrap();

    f(mgr)
}

pub fn with_manager_mut<F, R>(f: F) -> R
where F: Fn(&mut Manager) -> R {
    let mgr_ref = unsafe { &GLOBAL_SCREENS };

    let mut guard = mgr_ref.write();
    let mgr = guard.get_mut().unwrap();

    f(mgr)
}