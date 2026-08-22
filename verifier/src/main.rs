// Decision note: stage 1 proves the toolchain, lockfile, and audit path only.
// The strict no-panic certificate schema and chain checking arrive in stage 6
// per docs/PLAN.md section 11.

/// Contract: exactly one argument accepted, "--version"; anything else exits 2.
pub fn run<I>(args: I) -> i32
where
    I: IntoIterator<Item = String>,
{
    match args.into_iter().next() {
        Some(arg) if arg == "--version" => {
            println!("zetaforge-verifier {}", env!("CARGO_PKG_VERSION"));
            0
        }
        _ => {
            eprintln!("usage: zetaforge-verifier --version");
            2
        }
    }
}

fn main() {
    std::process::exit(run(std::env::args().skip(1)));
}

#[cfg(test)]
mod tests {
    use super::run;

    fn none() -> impl Iterator<Item = String> {
        Vec::new().into_iter()
    }

    #[test]
    fn version_flag_is_accepted() {
        assert_eq!(run(vec!["--version".to_string()].into_iter()), 0);
    }

    #[test]
    fn bare_invocation_is_rejected() {
        assert_eq!(run(none()), 2);
    }

    #[test]
    fn unknown_command_is_rejected() {
        assert_eq!(run(vec!["verify".to_string()].into_iter()), 2);
    }
}
