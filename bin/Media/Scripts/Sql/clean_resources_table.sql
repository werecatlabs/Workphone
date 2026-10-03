-- Step 1: Identify duplicates
SELECT uuid, COUNT(*)
FROM resources
GROUP BY uuid
HAVING COUNT(*) > 1;

-- Step 2: Choose which record to keep (e.g., based on the smallest ID)
-- In this example, we'll keep the record with the smallest ID
CREATE TABLE temp AS
SELECT MIN(id) AS keep_id, uuid
FROM resources
GROUP BY uuid;

-- Step 3: Delete duplicates
DELETE FROM resources
WHERE id NOT IN (SELECT keep_id FROM temp);

-- Step 4: Drop the temporary table
DROP TABLE temp;
