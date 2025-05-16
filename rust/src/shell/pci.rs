use alloc::string::String;

use super::ShellContext;

pub static PCI_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("pci", pci, Some("Lists PCI devices"));

pub fn pci(_context: &mut ShellContext, _args: &[String]) -> Result<(), usize> {
    crate::system::pci::pci_print_list();

    Ok(())
}
