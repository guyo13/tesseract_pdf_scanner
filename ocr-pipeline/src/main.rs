use clap::Parser;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[arg(short, long)]
    input: String,

    #[arg(short, long)]
    keywords: String,

    #[arg(long, default_value_t = 1)]
    start_page: usize,

    #[arg(long, default_value_t = usize::MAX)]
    end_page: usize,
}


fn main() {
    let args = Args::parse();
    println!("{:?}", args);
}
