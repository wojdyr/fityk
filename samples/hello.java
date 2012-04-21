// Example of using libfityk from Java.
// To run this example:
// - compile dynamic module for Java: after compiling fityk go to src/,
//   run "make java" with proper paths. On Fedora Linux it means:
//   $ JAVAINC=/usr/lib/jvm/java/include
//   $ make java CPPFLAGS="-I$JAVAINC -I/$JAVAINC/linux"
//   Put fitykJ.so into a directory in dynamic library path.
//   (libfityk.so should also be installed.)
// - java files in swig/java are in "package fityk", so compile the files:
//   $ cd ..../src/swig/java
//   $ javac *.java
//   and put them in a directory named fityk in CLASSPATH.
//   Link will also do:
//   $ cd ..../samples
//   $ ln -s .../src/swig/java ./fityk; export CLASSPATH=.
//   Now you can try this sample:
//   $ java hello

import fityk.Fityk;

public class hello extends Fityk {
    static {
        System.loadLibrary("fitykJ");
    }

    public static void main(String argv[]) {
        Fityk f = new Fityk();
        System.out.println(f.get_info("version"));
        System.out.println("ln(2) = " + f.get_info("version"));

        hello g = new hello("nacl01.dat");
        g.run();
        g.save_session("tmp_save.fit");
    }

    // Actual class content starts here.
    // It's named `hello' and kept in the same file as main because
    // we like each sample to be in a single file.
    String filename;

    public hello (String filename) {
        this.filename = filename;
        execute(String.format("@0 < '%s'", filename));
        System.out.println("Data info:" + get_info("data", 0));
    }

    public void run() {
        execute("guess %gauss = Gaussian");
        System.out.println("Fitting " + filename + " ...");
        execute("fit");
        System.out.println("WSSR=" + get_wssr());
        System.out.println("Gaussian center: " +
                           calculate_expr("%gauss.center"));
    }

    public void save_session(String sessionFilename) {
        execute(String.format("info state >'%s'", sessionFilename));
    }
}

