use super::*;

const INITIAL_PATH: &str = "R:/";

#[test]
fn creation_from_path() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let finalized = path.unwrap().to_string();

    assert_eq!(&finalized, INITIAL_PATH);
}

#[test]
fn creation_from_letter() {
    let path = Path::with_letter('R');
    let finalized = path.to_string();

    assert_eq!(&finalized, INITIAL_PATH);
}

#[test]
fn apply_basic() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/4");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/1/2/3/4");
}

#[test]
fn apply_with_trailing_slash() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/4/");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/1/2/3/4");
}

#[test]
fn apply_absolute() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("R:/a/b/c/d/e/");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/a/b/c/d/e");
}

#[test]
fn apply_with_parent_modifiers() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/../../a/2/b");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/1/a/2/b");
}

#[test]
fn apply_with_all_modifiers() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/2/3/../../a/b/./.././");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/1/a");
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
fn parent_folder_in_root_always_root() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("1/../../../../");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/");
}

#[test]
fn file_access() {
    let path = Path::from_path(INITIAL_PATH);

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("secrets.txt");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/secrets.txt");
}

#[test]
fn untrailed_slash() {
    let path = Path::from_path("R:/heaven");

    assert!(path.is_some());

    let mut path = path.unwrap();

    path.apply("file.txt");

    let finalized = path.to_string();

    assert_eq!(&finalized, "R:/heaven/file.txt");
}
