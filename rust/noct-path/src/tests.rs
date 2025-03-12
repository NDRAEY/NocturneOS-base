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
fn apply_01() {
	let mut path = Path::from_path(INITIAL_PATH);

	assert!(path.is_some());

	let mut path = path.unwrap();

	path.apply("1/2/3/4");

	let finalized = path.to_string();

	assert_eq!(&finalized, "R:/1/2/3/4");
}
