import sqlite3

conn = None

def print_table(tablename):
    c = conn.cursor()
    print("===== {} =====".format(tablename))
    c.execute("PRAGMA table_info('{}')".format(tablename))
    for _, col_name, _, _, _, _ in c.fetchall():
        print(col_name, end=" | ")
    print("")
    c.execute("SELECT * FROM '{}'".format(tablename))
    for e in c.fetchall():
        for c in e:
            print(str(c).encode("utf-8"), end=' | ')
        print("")

def connect_db(db):
    global conn
    try:
        open(db, "rb")
    except OSError:
        conn = sqlite3.connect(db)
        init_db()
    else:
        conn = sqlite3.connect(db)

def init_db():
    c = conn.cursor()
    c.execute("DROP TABLE IF EXISTS MEMBERS;")
    c.execute("DROP TABLE IF EXISTS MESSAGES;")
    c.execute("""
    CREATE TABLE MEMBERS (
    member_id integer PRIMARY KEY AUTOINCREMENT,
    member_name text,
    member_hash blob
    );
    """)
    c.execute("""
    CREATE TABLE MESSAGES (
    message_id integer PRIMARY KEY AUTOINCREMENT,
    member_name text,
    message_content blob
    );""")
    add_test_users()
    conn.commit()

def add_test_users():
    SERVER_HA = b'\x9c\xf1\xa4\xa6\xb9\xd3\xa3<\xc5\xca\xb6\xd3o"[\xca\xfd\x94\xccaQ\xc8QR8\x96H\xb0\x1c\xc6\xa7M'
    CLIENT_HA = b'L\xa0\x8e\x04\xac\xda\x0fh\x9d\xa1A\xbb\x10\xfc\xfePI\xfa\x19\xda\x9bW\xf0\x8b\x91\xcd%j\x84UF\x04'
    add_user('Jean', CLIENT_HA)
    add_user('François', SERVER_HA)

def add_user(user_name, user_hash):
    print("adding user", user_name, user_hash)
    c = conn.cursor()
    c.execute("INSERT INTO MEMBERS (member_name, member_hash) VALUES(:name,:hash)",
                  {"name": user_name, "hash": memoryview(user_hash)})
    conn.commit()

def get_user_hash(user_name):
    print("user name=({})".format(user_name))
    c = conn.cursor()
    c.execute("SELECT member_hash FROM MEMBERS WHERE member_name=:name",
              {"name": user_name})
    return c.fetchone()[0]

def add_msg(user_name, content):
    print("adding msg ({}), ({})".format(user_name, content))
    c = conn.cursor()
    c.execute("INSERT INTO MESSAGES (member_name, message_content) VALUES (:name, :content)", 
              {"name": user_name, "content": content})
    conn.commit()