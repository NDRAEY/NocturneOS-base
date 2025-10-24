#![no_std]

use alloc::{collections::linked_list::LinkedList, string::String};
use lazy_static::lazy_static;
use noct_timer::timestamp;
use spin::RwLock;

extern crate alloc;

pub mod c_api;
pub enum LogType {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    SUCCESS,
}

pub struct LoggingEntry {
    pub time: usize,
    pub m_type: LogType,
    pub message: String,
}

impl LoggingEntry {
    pub fn debug(message: String) -> Self {
        Self {
            time: timestamp(),
            m_type: LogType::DEBUG,
            message,
        }
    }

    pub fn info(message: String) -> Self {
        Self {
            time: timestamp(),
            m_type: LogType::INFO,
            message,
        }
    }

    pub fn warn(message: String) -> Self {
        Self {
            time: timestamp(),
            m_type: LogType::WARNING,
            message,
        }
    }

    pub fn error(message: String) -> Self {
        Self {
            time: timestamp(),
            m_type: LogType::ERROR,
            message,
        }
    }

    pub fn ok(message: String) -> Self {
        Self {
            time: timestamp(),
            m_type: LogType::SUCCESS,
            message,
        }
    }
}

pub struct Logger {
    entries: LinkedList<LoggingEntry>,
}

impl Logger {
    pub fn new() -> Self {
        Self {
            entries: LinkedList::new(),
        }
    }

    pub fn push(&mut self, entry: LoggingEntry) {
        self.entries.push_back(entry);
    }

    #[inline]
    pub const fn get(&self) -> &LinkedList<LoggingEntry> {
        &self.entries
    }
}

lazy_static! {
    pub static ref INTERNAL_LOGGER: RwLock<Logger> = RwLock::new(Logger::new());
}

pub fn debug(message: String) {
    INTERNAL_LOGGER.write().push(LoggingEntry::debug(message));
}

pub fn info(message: String) {
    INTERNAL_LOGGER.write().push(LoggingEntry::info(message));
}

pub fn warn(message: String) {
    INTERNAL_LOGGER.write().push(LoggingEntry::warn(message));
}

pub fn error(message: String) {
    INTERNAL_LOGGER.write().push(LoggingEntry::error(message));
}

pub fn ok(message: String) {
    INTERNAL_LOGGER.write().push(LoggingEntry::ok(message));
}

pub fn get_logs() -> spin::RwLockReadGuard<'static, Logger> {
    INTERNAL_LOGGER.read()
}

#[macro_export]
macro_rules! debug {
    ($($arg:tt)*) => {
        $crate::debug(alloc::format!("{}", format_args!($($arg)*)));
    };
}

#[macro_export]
macro_rules! log {
    ($($arg:tt)*) => {
        $crate::info(alloc::format!("{}", format_args!($($arg)*)));
    };
}

#[macro_export]
macro_rules! error {
    ($($arg:tt)*) => {
        $crate::error(alloc::format!("{}", format_args!($($arg)*)));
    };
}

#[macro_export]
macro_rules! warn {
    ($($arg:tt)*) => {
        $crate::warn(alloc::format!("{}", format_args!($($arg)*)));
    };
}

#[macro_export]
macro_rules! ok {
    ($($arg:tt)*) => {
        $crate::ok(alloc::format!("{}", format_args!($($arg)*)));
    };
}
