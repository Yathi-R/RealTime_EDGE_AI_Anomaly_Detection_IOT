CREATE TABLE mqtt_to_mssql_table
(
    topic                    NVARCHAR(MAX),
    payload_utf8             NVARCHAR(MAX),
    payload_base64           NVARCHAR(MAX),
    qos                      NVARCHAR(MAX),
    retain                   BIT,
    packet_id                INT,
    payload_format_indicator NVARCHAR(MAX),
    response_topic           NVARCHAR(MAX),
    correlation_data_utf8    NVARCHAR(MAX),
    correlation_data_base64  NVARCHAR(MAX),
    arrival_timestamp        DATETIME2, -- DATETIMEOFFSET would be preferable but is currently not supported by bulk copy,
    -- read more https://learn.microsoft.com/en-us/sql/connect/jdbc/use-bulk-copy-api-batch-insert-operation?view=sql-server-ver16#known-limitations
    arrival_timestamp_ms     BIGINT,
    user_properties          NVARCHAR(MAX),
    user_property1           NVARCHAR(MAX)
);
