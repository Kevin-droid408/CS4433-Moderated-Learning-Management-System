
-- Run only once against the existing database.
-- Back up database/lms.db before executing.

BEGIN IMMEDIATE TRANSACTION;

ALTER TABLE users ADD COLUMN username TEXT;

CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username
    ON users(username);

COMMIT;
