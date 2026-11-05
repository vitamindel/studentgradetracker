// cloud.js - login (Firebase Authentication) and the cloud database (Cloud Firestore).
// Database layout (firestore.rules decides who can read what):
//   accounts/{uid}               role: "teacher" or "student" (+ which student record)
//   users/{teacherUid}           a teacher's class settings
//   users/{teacherUid}/students/{id}   one student, with their joinCode
//   joinCodes/{code}             code -> teacher + student, used to make a student account
import { firebaseConfig } from "./firebase-config.js";

const SDK = "https://www.gstatic.com/firebasejs/12.6.0";

export const cloudEnabled = Boolean(firebaseConfig.apiKey);

let fb = null;     // the Firebase functions, once loaded
let auth = null;
let db = null;

// What the database holds right now, so a save only writes what changed.
let savedSettings = "";
let savedStudents = new Map();   // id -> studentKey()
let savedCodes = new Set();      // joinCodes that exist in the database
let codes = new Map();           // id -> joinCode (kept after a delete, so undo gets the same code)

// Downloads the Firebase SDK (only when cloudEnabled) and connects.
export async function initCloud() {
  const [appSdk, authSdk, dbSdk] = await Promise.all([
    import(`${SDK}/firebase-app.js`),
    import(`${SDK}/firebase-auth.js`),
    import(`${SDK}/firebase-firestore.js`),
  ]);
  fb = { ...authSdk, ...dbSdk };
  const app = appSdk.initializeApp(firebaseConfig);
  auth = authSdk.getAuth(app);
  db = dbSdk.getFirestore(app);
  if (firebaseConfig.projectId.startsWith("demo-")) {   // "demo-" projects run on the local emulators (tests)
    authSdk.connectAuthEmulator(auth, "http://127.0.0.1:9099", { disableWarnings: true });
    dbSdk.connectFirestoreEmulator(db, "127.0.0.1", 8080);
  }
}

// ----- login -----
export const onUserChanged = (callback) => fb.onAuthStateChanged(auth, callback);
export const signIn = (email, password) => fb.signInWithEmailAndPassword(auth, email, password);
export const signUp = (email, password) => fb.createUserWithEmailAndPassword(auth, email, password);
export const signInWithGoogle = () => fb.signInWithPopup(auth, new fb.GoogleAuthProvider());
export const resetPassword = (email) => fb.sendPasswordResetEmail(auth, email);
export const signOutUser = () => fb.signOut(auth);

// Firebase error codes -> messages a person can act on.
export function friendlyError(err) {
  const messages = {
    "auth/invalid-email": "That email address doesn't look right.",
    "auth/missing-email": "Type your email address.",
    "auth/missing-password": "Type your password.",
    "auth/invalid-credential": "Wrong email or password.",
    "auth/wrong-password": "Wrong email or password.",
    "auth/user-not-found": "Wrong email or password.",
    "auth/email-already-in-use": "There's already an account with that email. Sign in instead.",
    "auth/weak-password": "Passwords need at least 6 characters.",
    "auth/popup-closed-by-user": "The Google window was closed before you finished signing in.",
    "auth/cancelled-popup-request": "The Google window was closed before you finished signing in.",
    "auth/popup-blocked": "Your browser blocked the Google window. Allow pop-ups for this site and try again.",
    "auth/network-request-failed": "Can't reach the internet. Check your connection and try again.",
    "auth/too-many-requests": "Too many tries. Wait a minute, then try again.",
    "auth/operation-not-allowed": "This sign-in method isn't turned on in the Firebase console.",
    "auth/unauthorized-domain": "This website's address isn't on Firebase's list of authorized domains.",
    "permission-denied": "The database said no. Check the Firestore security rules.",
    "unavailable": "Can't reach the database right now. Check your connection.",
    "gradebook/bad-code": "That code doesn't match any student. Check it with your teacher.",
    "gradebook/no-code": "Type the student code your teacher gave you.",
  };
  return messages[err && err.code] || (err && err.message) || "Something went wrong.";
}

// ----- accounts and student codes -----
const CODE_CHARS = "ABCDEFGHJKMNPQRSTUVWXYZ23456789";   // no 0/O or 1/I/L mix-ups

function newCode() {
  const bytes = crypto.getRandomValues(new Uint8Array(8));
  return Array.from(bytes, (b) => CODE_CHARS[b % CODE_CHARS.length]).join("");
}

// The student's login code (made the first time it's needed, saved with the student).
export function codeFor(id) {
  const key = String(id);
  if (!codes.has(key)) codes.set(key, newCode());
  return codes.get(key);
}
// A new roster (demo, clear, import) reuses IDs from 1001, so everyone gets a new code
// and the old ones stop working. Otherwise a new student could inherit someone else's code.
export function resetCodes() {
  codes = new Map();
}

export const formatCode = (code) => `${code.slice(0, 4)}-${code.slice(4)}`;
export const cleanCode = (text) => text.toUpperCase().replace(/[^A-Z0-9]/g, "");

// Which student a code belongs to, or null. Works before signing in.
export async function lookupCode(code) {
  if (!/^[A-Z0-9]{8}$/.test(code)) return null;
  const snap = await fb.getDoc(fb.doc(db, "joinCodes", code));
  return snap.exists() ? snap.data() : null;
}

export async function getAccount(uid) {
  const snap = await fb.getDoc(fb.doc(db, "accounts", uid));
  return snap.exists() ? snap.data() : null;
}

// Saves this person's role. A student account is tied to one student record by its code.
export async function setAccount(user, role, code = "") {
  let account = { role: "teacher", email: user.email || "" };
  if (role === "student") {
    const link = await lookupCode(code);
    if (!link) throw Object.assign(new Error("bad code"), { code: "gradebook/bad-code" });
    account = { role: "student", email: user.email || "", teacherUid: link.teacherUid, studentId: link.studentId, code };
  }
  await fb.setDoc(fb.doc(db, "accounts", user.uid), account);
  return account;
}

// Student view: calls onData with the class settings + this one student, now and after every change.
export async function watchStudent(account, onData, onError) {
  const classRef = fb.doc(db, "users", account.teacherUid);
  const classDoc = await fb.getDoc(classRef);
  const settings = classDoc.exists() ? classDoc.data() : {};
  return fb.onSnapshot(fb.doc(classRef, "students", account.studentId), (snap) => {
    if (!snap.exists()) {
      onError(Object.assign(new Error("removed"), { code: "permission-denied" }));
      return;
    }
    const s = snap.data();
    onData({ ...settings, students: [{ ...s, grades: Array.isArray(s.grades) ? s.grades : [] }] });
  }, onError);
}

// ----- the teacher's class -----
const studentKey = (s) => JSON.stringify([s.id, s.name, s.average, s.letter, s.status, s.joinCode || null,
  s.grades.map((g) => [g.label, g.type, g.score])]);
const settingsKey = (c) => JSON.stringify([c.className, c.passCutoff, c.needsCutoff]);

// Reads this user's class, or returns null if they have never saved one.
export async function loadClass(uid) {
  const userRef = fb.doc(db, "users", uid);
  const [userDoc, studentDocs] = await Promise.all([
    fb.getDoc(userRef),
    fb.getDocs(fb.collection(userRef, "students")),
  ]);
  const students = studentDocs.docs.map((d) => d.data())
    .map((s) => ({ ...s, grades: Array.isArray(s.grades) ? s.grades : [] }))
    .sort((a, b) => a.id - b.id);
  savedStudents = new Map(students.map((s) => [String(s.id), studentKey(s)]));
  const withCodes = students.filter((s) => s.joinCode);
  codes = new Map(withCodes.map((s) => [String(s.id), s.joinCode]));
  savedCodes = new Set(withCodes.map((s) => s.joinCode));
  if (!userDoc.exists()) {
    savedSettings = "";
    return null;
  }
  const data = { ...userDoc.data(), students };
  savedSettings = settingsKey(data);
  return data;
}

// Writes the class in one batch (all or nothing): changed students, removed students, settings.
export async function saveClass(uid, data) {
  const userRef = fb.doc(db, "users", uid);
  const batch = fb.writeBatch(db);
  let changes = 0;

  const current = new Map();
  for (const s of data.students) {
    const id = String(s.id);
    const student = { ...s, joinCode: codeFor(id) };
    current.set(id, studentKey(student));
    if (savedStudents.get(id) !== current.get(id)) {
      batch.set(fb.doc(userRef, "students", id), student);
      if (!savedCodes.has(student.joinCode)) {
        batch.set(fb.doc(db, "joinCodes", student.joinCode), { teacherUid: uid, studentId: id });
      }
      changes++;
    }
  }
  for (const id of savedStudents.keys()) {
    if (!current.has(id)) {
      batch.delete(fb.doc(userRef, "students", id));
      changes++;
    }
  }
  const liveCodes = new Set([...current.keys()].map((id) => codes.get(id)));
  for (const code of savedCodes) {
    if (!liveCodes.has(code)) {
      batch.delete(fb.doc(db, "joinCodes", code));   // a removed student's code stops working
      changes++;
    }
  }
  if (changes === 0 && settingsKey(data) === savedSettings) return false;   // nothing new

  batch.set(userRef, {
    className: data.className,
    passCutoff: data.passCutoff,
    needsCutoff: data.needsCutoff,
    studentCount: data.students.length,
    updatedAt: fb.serverTimestamp(),
  });
  await batch.commit();
  savedStudents = current;
  savedSettings = settingsKey(data);
  savedCodes = liveCodes;
  return true;
}
