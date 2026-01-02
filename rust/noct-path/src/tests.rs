use super::*;

const INITIAL_PATH: &str = "R:/";

#[test]
fn creation_from_path() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let finalized = path.unwrap();

    assert_eq!(finalized, INITIAL_PATH);
}

#[test]
fn creation_from_letter() {
    let path = Path::with_letter('R');

    assert_eq!(path, INITIAL_PATH);
}

#[test]
fn apply_basic() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/4");

    assert_eq!(path, "R:/1/2/3/4");
}

#[test]
fn apply_with_trailing_slash() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/4/");

    assert_eq!(path, "R:/1/2/3/4");
}

#[test]
fn apply_absolute() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("R:/a/b/c/d/e/");

    assert_eq!(path, "R:/a/b/c/d/e");
}

#[test]
fn apply_with_parent_modifiers() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/../../a/2/b");

    assert_eq!(path, "R:/1/a/2/b");
}

#[test]
fn apply_with_all_modifiers() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/../../a/b/./.././");

    assert_eq!(path, "R:/1/a");
}

#[test]
fn apply_with_dup_slashes() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("//1/2/3/4/5//");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/1/2/3/4/5");
}

#[test]
fn apply_with_modifiers_and_dup_slashes() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/../4/5/../6//7/..//8//9/");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/1/2/4/6/8/9");
}

#[test]
fn parent_folder_to_root() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/4/../../../../");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/");
}

#[test]
fn parent_folder_simple() {
    let path = Path::from_path("R:/1/");

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("../");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/");
}

#[test]
fn parent_folder_in_root_always_root() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/../../../../");

    assert_eq!(path, "R:/");
}

#[test]
fn file_access() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("secrets.txt");

    assert_eq!(path, "R:/secrets.txt");
}

#[test]
fn untrailed_slash() {
    let path = Path::from_path("R:/heaven");

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("file.txt");

    assert_eq!(path, "R:/heaven/file.txt");
}


#[test]
fn ambiguous_dots() {
    let path = Path::from_path("rd0:/");

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply(".");

    assert_eq!(path, "rd0:/");
}

#[test]
fn disk_and_path() {
    let path = Path::from_path("rd0:/").unwrap();

    assert_eq!(path.disk_name(), "rd0");
    assert_eq!(path.path_part(), "");


    let path = Path::from_path("nvme0:/a/bc/def").unwrap();

    assert_eq!(path.disk_name(), "nvme0");
    assert_eq!(path.path_part(), "a/bc/def");
}

#[test]
fn absolute_or_not() {
    assert_eq!(Path::is_absolute("rd0:/"), true);
    assert_eq!(Path::is_absolute("rd0:/a/b/c/"), true);
    assert_eq!(Path::is_absolute("rd0:/./../."), true);
    assert_eq!(Path::is_absolute("rd0:/1.txt"), true);
    
    assert_eq!(Path::is_absolute("rd0:"), false);
    assert_eq!(Path::is_absolute(":/a/b/c.txt"), false);
    assert_eq!(Path::is_absolute("../../:/a/b.c"), false);
}
